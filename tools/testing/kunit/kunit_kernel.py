# SPDX-License-Identifier: GPL-2.0

import logging
import subprocess
import os

import kunit_config
import kunit_parser

KCONFIG_PATH = '.config'

from collections import namedtuple

ConfigResult = namedtuple('ConfigResult', ['status','info'])

BuildResult = namedtuple('BuildResult', ['status','info'])

class ConfigStatus(object):
	SUCCESS = 'SUCCESS'
	FAILURE = 'FAILURE'

class BuildStatus(object):
	SUCCESS = 'SUCCESS'
	FAILURE = 'FAILURE'

class ConfigError(Exception):
	"""Represents an error trying to configure the Linux kernel."""


class BuildError(Exception):
	"""Represents an error trying to build the Linux kernel."""


class LinuxSourceTreeOperations(object):
	"""An abstraction over command line operations performed on a source tree."""

	def make_mrproper(self):
		try:
			subprocess.check_output(['make', 'mrproper'])
		except OSError as e:
			raise ConfigError('Could not call make command: ' + str(e))
		except subprocess.CalledProcessError as e:
			raise ConfigError(e.output.decode())

	def make_olddefconfig(self):
		try:
			subprocess.check_output(['make', 'ARCH=um', 'olddefconfig'])
		except OSError as e:
			raise ConfigError('Could not call make command: ' + str(e))
		except subprocess.CalledProcessError as e:
			raise ConfigError(e.output.decode())

	def make(self, jobs):
		try:
			subprocess.check_output(['make', 'ARCH=um', '--jobs=' + str(jobs)])
		except OSError as e:
			raise BuildError('Could not call execute make: ' + str(e))
		except subprocess.CalledProcessError as e:
			raise BuildError(e.output.decode())

	def linux_bin(self, params, timeout):
		"""Runs the Linux UML binary. Must be named 'linux'."""
		process = subprocess.Popen(
			['./linux'] + params,
			stdin=subprocess.PIPE,
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE)
		timed_out = False
		try:
			process.wait(timeout=timeout)
		except subprocess.TimeoutExpired:
			process.terminate()
			timed_out = True
		output, _ = process.communicate()
		output = output.decode('ascii')

		if timed_out:
			output += kunit_parser.TIMED_OUT_LOG_ENTRY + '\n'

		return output

def throw_error_if_not_subset(expected_superset: kunit_config.Kconfig,
			      expected_subset: kunit_config.Kconfig) -> None:
	if not expected_subset.is_subset_of(expected_superset):
		missing = expected_subset.entries() - expected_superset.entries()
		message = 'Provided Kconfig contains fields not in validated .config: %s' % (
			', '.join([str(e) for e in missing]),
		)
		raise ConfigError(message)

class ExtKunitconfigGenerator():
    '''
    Generate a new kunitconfig what you interested in.
    kunitconfigs/kunitconfig : original kunitconfig (when there is no external-config opt)
    kunitconfigs/.kunitconfig : re-generated kunitconfig
    '''

    def __init__(self, ex_config, path='.'):
        self.rootdir = os.path.join(path, 'kunitconfigs')
        self.final_config_file = os.path.join(self.rootdir, '.kunitconfig')
        orig_config_file = os.path.join(self.rootdir, 'kunitconfig')
        conf = []
        merge_configs = self.gen_merge_config(ex_config)

        conf.append(self.read_config(orig_config_file))
        for c in merge_configs:
            conf.append(self.read_config(c))
        self.write_config(self.final_config_file, conf)

    def read_config(self, fname):
        with open(fname, 'r') as fp:
            ret = fp.read()
        return ret

    def write_config(self, fname, li):
        with open(fname, 'w') as fp:
            for l in li:
                fp.write(l)

    def get_sub_config(self, parent):
        childs = []
        kuconf_list = os.listdir(self.rootdir)
        for f in kuconf_list:
            srch = '.' + parent
            if srch in f.split('kunitconfig')[1]:
                #FIXME: matching in the right side
                childs.append(os.path.join(self.rootdir, f))
        return childs

    def gen_merge_config(self, ex_li):
        ret = []
        for module in ex_li:
            sub_conf = self.get_sub_config(module)
            for c in sub_conf:
                ret.append(c)
        return ret


class LinuxSourceTree(object):
	"""Represents a Linux kernel source tree with KUnit tests."""

	def __init__(self,
			     kconfig_provider=kunit_config.KunitConfigProvider(),
				 linux_build_operations=LinuxSourceTreeOperations()):
		self._kconfig = kconfig_provider.get_kconfig()
		self._ops = linux_build_operations
		self.kconf_provider = kconfig_provider

	def make_external_config(self, ex_config):
		if not ex_config:
			return
		conf = ExtKunitconfigGenerator(ex_config).final_config_file
		self._kconfig = self.kconf_provider.get_kconfig(conf)

	def clean(self):
		try:
			self._ops.make_mrproper()
		except ConfigError as e:
			logging.error(e)
			return False
		return True

	def build_config(self):
		self._kconfig.write_to_file(KCONFIG_PATH)
		try:
			self._ops.make_olddefconfig()
		except ConfigError as e:
			logging.error(e)
			return ConfigResult(ConfigStatus.FAILURE, str(e))
		validated_kconfig = kunit_config.Kconfig()
		validated_kconfig.read_from_file(KCONFIG_PATH)
		try:
			throw_error_if_not_subset(expected_subset=self._kconfig,
						  expected_superset=validated_kconfig)
		except ConfigError as e:
			logging.error(e)
			return ConfigResult(ConfigStatus.FAILURE, str(e))
		return ConfigResult(ConfigStatus.SUCCESS, 'Build config!')

	def build_reconfig(self):
		"""Creates a new .config if it is not a subset of the kunitconfig."""
		if os.path.exists(KCONFIG_PATH):
			existing_kconfig = kunit_config.Kconfig()
			existing_kconfig.read_from_file(KCONFIG_PATH)
			if not self._kconfig.is_subset_of(existing_kconfig):
				print('Regenerating .config ...')
				os.remove(KCONFIG_PATH)
				return self.build_config()
			else:
				return ConfigResult(ConfigStatus.SUCCESS, 'Already built.')
		else:
			print('Generating .config ...')
			return self.build_config()

	def build_um_kernel(self, jobs):
		try:
			self._ops.make_olddefconfig()
			self._ops.make(jobs)
		except (ConfigError, BuildError) as e:
			logging.error(e)
			return BuildResult(BuildStatus.FAILURE, str(e))
		used_kconfig = kunit_config.Kconfig()
		used_kconfig.read_from_file(KCONFIG_PATH)
		try:
			throw_error_if_not_subset(expected_subset=self._kconfig,
						  expected_superset=used_kconfig)
		except ConfigError as e:
			logging.error(e)
			return ConfigResult(ConfigStatus.FAILURE, str(e))
		return BuildResult(BuildStatus.SUCCESS, 'Built kernel!')

	def run_kernel(self, args=[], timeout=None):
		args.extend(['mem=256M kunit_shutdown=1'])
		raw_log = self._ops.linux_bin(args, timeout)
		with open('test.log', 'w') as f:
			for line in raw_log.split('\n'):
				f.write(line.rstrip() + '\n')
				yield line.rstrip()
