#include <kunit/test.h>
#include <kunit/mock.h>
/* #include <linux/example/example_driver.h> */

static void kunit_test(struct test *test)
{
	/*
	struct example_driver_drvdata *p = test->priv;

	EXPECT_EQ(test, -EINVAL,  example_driver_set_intensity(p, 99999));
	EXPECT_FALSE(test, example_driver_set_enable(p, true));
	*/
	return;
}

static void kunit_test2(struct test *test)
{
	/*
	struct example_driver_drvdata *p = test->priv;

	EXPECT_FALSE(test, example_driver_set_enable(p, false));
	EXPECT_EQ(test, -EINVAL,  example_set_frequency(p, EXAMPLE_FREQ_MAX));
	*/
	return;
}


static int kunit_test_init(struct test *test)
{
	/*
	test->priv = a_struct_pointer;
	if (!test->priv)
		return -ENOMEM;
	*/

	return 0;
}

static void kunit_test_exit(struct test *test)
{
	return;
}

static struct test_case kunit_test_cases[] = {
	TEST_CASE(kunit_test),
	TEST_CASE(kunit_test2),
	{},
};

static struct test_module kunit_test_module = {
	.name = "kunit-test",
	.init = kunit_test_init,
	.exit = kunit_test_exit,
	.test_cases = kunit_test_cases,
};
module_test(kunit_test_module);
