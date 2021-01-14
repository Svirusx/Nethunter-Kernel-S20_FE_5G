obj-$(CONFIG_KUNIT)		+= test.o mock.o common-mocks.o strerror.o \
  string-stream.o test-stream.o test-executor.o try-catch.o
obj-$(CONFIG_KUNIT_TEST)		+= \
  test-mock.o mock-macro-test.o mock-test.o strerror-test.o \
  string-stream-test.o test-stream-test.o test-test.o
obj-$(CONFIG_KUNIT_DEATH_TEST)	+= test-death-test.o
obj-$(CONFIG_KUNIT_EXAMPLE_TEST)	+= example-test.o
obj-$(CONFIG_KUNIT_MANAGER)	+= kunit_manager.o

GCOV_PROFILE := $(CONFIG_KUNIT)
obj-$(CONFIG_KUNIT) += test/
