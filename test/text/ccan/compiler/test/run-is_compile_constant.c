#include <ccan/compiler/compiler.h>
#include <ccan/tap/tap.h>

int main(int argc, char *argv[])
{
	plan_tests(2);

	ok1(!IS_COMPILE_CONSTANT(argc));
#if HAVE_BUILTIN_CONSTANT_P
	ok1(IS_COMPILE_CONSTANT(7));
#else
	pass("If !HAVE_BUILTIN_CONSTANT_P, IS_COMPILE_CONSTANT always false");
#endif
	return exit_status();
}
