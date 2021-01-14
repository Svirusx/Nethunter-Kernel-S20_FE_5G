#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/reboot.h>
#include <kunit/test.h>

extern char __test_modules_start;
extern char __test_modules_end;

static int kunit_shutdown;
core_param(kunit_shutdown, kunit_shutdown, int, 0644);

static bool test_run_all_tests(void)
{
	struct test_module **module;
	struct test_module ** const test_modules_start =
			(struct test_module **) &__test_modules_start;
	struct test_module ** const test_modules_end =
			(struct test_module **) &__test_modules_end;
	bool has_test_failed = false;

	for (module = test_modules_start; module < test_modules_end; ++module) {
		if (test_run_tests(*module))
			has_test_failed = true;
	}

	if (kunit_shutdown)
		kernel_halt();

	return !has_test_failed;
}


int test_executor_init(void)
{
	if (test_run_all_tests())
		return 0;
	else
		return -EFAULT;
}
