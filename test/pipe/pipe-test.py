#!/usr/bin/env python3
"""pty-based pipe tests for vis.

The plain `test/vis` harness runs vis with stdin from a file and no
controlling terminal, which cannot exercise the terminal/process-group
interaction of `: >cmd`. These tests drive vis through a real pty and
check two filter shapes against the process semantics reported in the
`fix/pipe-filter-daemons` PR:

  1. sudo/doas shape (mock-sudo): a filter that reads and writes the
     controlling terminal (password prompt) before consuming stdin.
     Without foreground handover the kernel SIGTTIN/SIGTTOU-stops the
     filter and the editor hangs; the typed password leaks into the
     editor instead of reaching the filter.

  2. wl-copy shape (mock-wlcopy): a filter that daemonizes while the
     daemon inherits the editor's pipe write ends, so no EOF ever
     arrives. The editor must reap the dead direct child and drain
     instead of blocking indefinitely.

Each test asserts that vis stays responsive (accepts `:q!`) and that
the filter actually did its work.
"""
import os
import pty
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time

VIS = sys.argv[1] if len(sys.argv) > 1 else "../../vis"
HERE = os.path.dirname(os.path.abspath(__file__))

failures = []


def check(name, ok, detail=""):
    print(f"{name:<40} {'PASS' if ok else 'FAIL'} {detail}")
    if not ok:
        failures.append(name)


def drain(fd, timeout=0.4):
    """Read everything available within a rolling timeout."""
    out = b""
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], max(0, end - time.time()))
        if not r:
            continue
        try:
            data = os.read(fd, 65536)
        except OSError:
            break
        if not data:
            break
        out += data
        end = time.time() + timeout
    return out


def send(fd, text, inter=0.04):
    """Type one char at a time so vis's input parser keeps up."""
    for ch in text:
        os.write(fd, ch.encode())
        time.sleep(inter)


def start_vis(fixture=None, extra_env=None):
    pid, fd = pty.fork()
    if pid == 0:
        os.environ["TERM"] = "vt100"
        os.environ["PATH"] = HERE + ":" + os.environ["PATH"]
        if extra_env:
            os.environ.update(extra_env)
        if fixture:
            os.execl(VIS, VIS, fixture)
        os.execl(VIS, VIS)
    return pid, fd


def vis_running(pid):
    try:
        w, _ = os.waitpid(pid, os.WNOHANG)
        return w == 0
    except ChildProcessError:
        return False


def quit_vis(fd, pid, timeout=3.0):
    """Send :q! and return True if vis exited within timeout."""
    send(fd, ":q!\r")
    end = time.time() + timeout
    while time.time() < end:
        drain(fd, 0.2)
        if not vis_running(pid):
            return True
    return False


def kill_tree(pid):
    try:
        os.kill(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    try:
        os.waitpid(pid, 0)
    except (ChildProcessError, OSError):
        pass


def with_tmpdir():
    d = tempfile.mkdtemp(prefix="vis-pipe-test.")
    return d, lambda: shutil.rmtree(d, ignore_errors=True)


def kill_logged_pids(logfile):
    """Kill every pid recorded by a mock in its log (lines start with $$).

    Guards against filters (or their daemons) outliving a wedged editor:
    the pty is gone but a stopped or setsid'd mock would otherwise linger
    after the test finishes.
    """
    try:
        with open(logfile) as f:
            for line in f:
                try:
                    pid = int(line.split(None, 1)[0])
                except (ValueError, IndexError):
                    continue
                try:
                    os.kill(pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
    except OSError:
        pass


# ---------------------------------------------------------------- test 1
def test_sudo_tee():
    """`, >mock-sudo tee OUT`: password prompt works, no hang, no leak."""
    tmp, cleanup = with_tmpdir()
    try:
        fixture = os.path.join(tmp, "fixture.txt")
        content = "hello vis pipe test\nsecond line\n"
        with open(fixture, "w") as f:
            f.write(content)
        out = os.path.join(tmp, "out.txt")
        pwfile = os.path.join(tmp, "pw.txt")
        log = os.path.join(tmp, "mock.log")

        pid, fd = start_vis(fixture, {"MOCK_LOG": log})
        try:
            time.sleep(1.0)
            drain(fd, 0.5)
            send(fd, f":, >{HERE}/mock-sudo tee {out}\r")
            time.sleep(1.0)
            drain(fd, 0.5)
            send(fd, "hunter2\r")
            time.sleep(1.5)
            screen = drain(fd, 1.0)

            check("sudo: vis accepts :q! after pipe", quit_vis(fd, pid))
            check("sudo: no password echoed into vis",
                  b"hunter2" not in screen,
                  "(password leaked into editor window)" if b"hunter2" in screen else "")
            try:
                with open(out) as f:
                    check("sudo: filter received range via tee",
                          f.read() == content)
            except OSError as e:
                check("sudo: filter received range via tee", False, str(e))
            try:
                with open(log) as f:
                    mocklog = f.read()
            except OSError as e:
                mocklog = ""
                check("sudo: mock log readable", False, str(e))
            check("sudo: mock got the password", "pw-ok" in mocklog, mocklog.strip())
            check("sudo: mock not SIGTTIN/SIGTTOU-stopped",
                  "stty-echo ok" in mocklog)
        finally:
            kill_tree(pid)
            kill_logged_pids(log)
    finally:
        cleanup()


# ---------------------------------------------------------------- test 2
def test_daemonizing_filter():
    """`, >mock-wlcopy OUT`: daemonizing filter must not hang the editor.

    Via cmd_pipeout (sam.c) the child's stdout is /dev/null, so this
    covers the reap-and-drain exit for the `: >wl-copy` command shape.
    The SIGCHLD wake itself is exercised by
    test_daemonizing_filter_through_pipe(), where the daemon holds the
    stdout pipe too and select() has nothing to report.
    """
    tmp, cleanup = with_tmpdir()
    try:
        fixture = os.path.join(tmp, "fixture.txt")
        with open(fixture, "w") as f:
            f.write("daemon filter test\n")
        pidfile = os.path.join(tmp, "daemon.pid")

        log = os.path.join(tmp, "mock.log")
        pid, fd = start_vis(fixture, {"MOCK_PIDFILE": pidfile, "MOCK_LOG": log})
        try:
            time.sleep(1.0)
            drain(fd, 0.5)
            send(fd, f":, >{HERE}/mock-wlcopy\r")
            # the pump must reap the dead child and drain on its own
            time.sleep(2.0)
            drain(fd, 0.5)
            check("wlcopy: vis accepts :q! (no hang)", quit_vis(fd, pid))
        finally:
            # kill any daemon the mock left behind
            try:
                with open(pidfile) as f:
                    os.kill(int(f.read().strip()), signal.SIGKILL)
            except (OSError, ValueError, ProcessLookupError):
                pass
            kill_tree(pid)
    finally:
        cleanup()


def test_daemonizing_filter_through_pipe():
    """`, |mock-wlcopy`: daemonizing filter holding BOTH stdout and stderr.

    Via cmd_filter (sam.c) the child's stdout is pout[1], so a daemonized
    grandchild keeps both pipe write ends open and no EOF ever arrives.
    select() then has nothing to report; only the SIGCHLD wake (self-pipe
    or EINTR) lets the pump reap the dead child and drain. This is the
    shape the `: >wl-copy`-style `>cmd` path (stdout = /dev/null) does NOT
    exercise.
    """
    tmp, cleanup = with_tmpdir()
    try:
        fixture = os.path.join(tmp, "fixture.txt")
        with open(fixture, "w") as f:
            f.write("daemon filter through pipe\n")
        pidfile = os.path.join(tmp, "daemon.pid")

        log = os.path.join(tmp, "mock.log")
        pid, fd = start_vis(fixture, {"MOCK_PIDFILE": pidfile, "MOCK_LOG": log})
        try:
            time.sleep(1.0)
            drain(fd, 0.5)
            send(fd, f":,|{HERE}/mock-wlcopy\r")  # NB: no space — ', |' parses as a bare range command
            # the pump must reap the dead child and drain on its own
            time.sleep(2.0)
            drain(fd, 0.5)
            check("wlcopy-pipe: vis accepts :q! (no hang)", quit_vis(fd, pid))
        finally:
            # kill the mock parent and any daemon it left behind
            kill_tree(pid)
            try:
                with open(pidfile) as f:
                    os.kill(int(f.read().strip()), signal.SIGKILL)
            except (OSError, ValueError, ProcessLookupError):
                pass
            kill_logged_pids(log)
    finally:
        cleanup()


# ---------------------------------------------------------------- test 3
def test_plain_filter_still_works():
    """`, >cat >OUT`: the ordinary non-tty filter path still functions."""
    tmp, cleanup = with_tmpdir()
    try:
        fixture = os.path.join(tmp, "fixture.txt")
        content = "plain filter content\n"
        with open(fixture, "w") as f:
            f.write(content)
        out = os.path.join(tmp, "out.txt")

        pid, fd = start_vis(fixture)
        try:
            time.sleep(1.0)
            drain(fd, 0.5)
            send(fd, f":, >cat >{out}\r")
            time.sleep(1.0)
            drain(fd, 0.5)
            check("plain: vis accepts :q! after pipe", quit_vis(fd, pid))
            try:
                with open(out) as f:
                    check("plain: filter received range", f.read() == content)
            except OSError as e:
                check("plain: filter received range", False, str(e))
        finally:
            kill_tree(pid)
    finally:
        cleanup()


test_sudo_tee()
test_daemonizing_filter()
test_daemonizing_filter_through_pipe()
test_plain_filter_still_works()

if failures:
    print(f"\n{len(failures)} test(s) failed: {', '.join(failures)}")
    sys.exit(1)
print("\nall pipe tests passed")
