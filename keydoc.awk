# Extract key documentation from vis config file.
function between(l, r, s) {
	if (s~l && s~r) {
		sub(l, "", s)
		sub(r, "", s)
	}
	return s
}
function remove(p, s) {
	sub(p, "", s)
	return s
}
function change(p, r, s) {
	sub(p, r, s)
	return s
}
BEGIN {
	block=0
	section=0
	divider="-------------------------------------------------------"
	colpos=28
}
/\[\] = \{/ { block=!match($0, /(syntaxes|editors|settings|colors)\[\]/) }
/};/ { block=0 }
{
	if (block) {
		line = $0
		if (line~/^static/) section = 1
		line = between("^static ", "\\[\\] = \\{.*$", line)
		line = remove("\\/\\*.*\\*\\/", line)
		line = between("\\{", "\\},$", line)
		line = between("^\\t", "$", line)
		line = between("^", " = \\{$", line)
		line = remove(",$", line)
		line = remove("\\{ ", line)
		line = change("\\},", ":", line)
		line = change(",[[:space:]]*\\{", ",", line)
		line = remove("[[:space:]]*\\}[[:space:]]*$", line)
		line = change(",[[:space:]]+C", ", C", line)
		line = change("\\\\t", "TAB", line)
		line = change("'\\)", ")", line)
		line = change("'\\)", ")", line)
		line = change("\\('", "(", line)
		line = change("'\\)", ")", line)
		line = change("\\('", "(", line)
		line = change("\\\\'", "'", line)
		line = change("NONE\\(", "KEY(", line)
		line = change("NONE\\(", "KEY(", line)
		if (line~/ : /) {
			split(line, a, " : ")
			a[1] = remove("[[:space:]]+$", a[1])
			line = sprintf("%*s : %s", colpos, a[1], a[2])
		}
		if (!(line~/^[[:space:]]*$/)) {
			if (section)
				print ""
			print line
			if (section)
				printf("%.*s\n", length(line), divider)
			section = 0
		}
	}
}
