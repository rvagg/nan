TOPLEVEL ?= $(dir $(lastword $(MAKEFILE_LIST)))
CPPLINT ?= $(TOPLEVEL)/cpplint.py
PYTHON ?= python
BUILDTYPE ?= Release
MODULES = symbols strings
SOURCES = $(MODULES:%=test/cpp/%.cpp)
ADDONS = $(MODULES:%=test/build/$(BUILDTYPE)/%.node)

LINT_SOURCES = \
	examples/async_pi_estimate/addon.cc \
	examples/async_pi_estimate/async.cc \
	examples/async_pi_estimate/async.h \
	examples/async_pi_estimate/pi_est.cc \
	examples/async_pi_estimate/pi_est.h \
	examples/async_pi_estimate/sync.cc \
	examples/async_pi_estimate/sync.h \
	nan.h \
	nan_callbacks.h \
	nan_callbacks_12_inl.h \
	nan_callbacks_pre_12_inl.h \
	nan_converters.h \
	nan_converters_43_inl.h \
	nan_converters_pre_43_inl.h \
	nan_define_own_property_helper.h \
	nan_implementation_12_inl.h \
	nan_implementation_pre_12_inl.h \
	nan_json.h \
	nan_maybe_43_inl.h \
	nan_maybe_pre_43_inl.h \
	nan_new.h \
	nan_object_wrap.h \
	nan_persistent_12_inl.h \
	nan_persistent_pre_12_inl.h \
	nan_private.h \
	nan_scriptorigin.h \
	nan_string_bytes.h \
	nan_weak.h \
	test/cpp/accessors.cpp \
	test/cpp/accessors2.cpp \
	test/cpp/asyncresource.cpp \
	test/cpp/asyncworker.cpp \
	test/cpp/asyncprogressworker.cpp \
	test/cpp/asyncprogressworkerstream.cpp \
	test/cpp/asyncprogressworkersignal.cpp \
	test/cpp/asyncprogressqueueworker.cpp \
	test/cpp/asyncprogressqueueworkerstream.cpp \
	test/cpp/asyncworkererror.cpp \
	test/cpp/buffer.cpp \
	test/cpp/bufferworkerpersistent.cpp \
	test/cpp/error.cpp \
	test/cpp/gc.cpp \
	test/cpp/indexedinterceptors.cpp \
	test/cpp/callbackcontext.cpp \
	test/cpp/converters.cpp \
	test/cpp/isolatedata.cpp \
	test/cpp/json-parse.cpp \
	test/cpp/json-stringify.cpp \
	test/cpp/makecallback.cpp \
	test/cpp/maybe.cpp \
	test/cpp/morenews.cpp \
	test/cpp/multifile1.cpp \
	test/cpp/multifile2.cpp \
	test/cpp/multifile2.h \
	test/cpp/namedinterceptors.cpp \
	test/cpp/nancallback.cpp \
	test/cpp/nannew.cpp \
	test/cpp/news.cpp \
	test/cpp/objectwraphandle.cpp \
	test/cpp/persistent.cpp \
	test/cpp/private.cpp \
	test/cpp/returnemptystring.cpp \
	test/cpp/returnnull.cpp \
	test/cpp/returnundefined.cpp \
	test/cpp/returnvalue.cpp \
	test/cpp/setcallhandler.cpp \
	test/cpp/settemplate.cpp \
	test/cpp/sleep.h \
	test/cpp/strings.cpp \
	test/cpp/symbols.cpp \
	test/cpp/threadlocal.cpp \
	test/cpp/trycatch.cpp \
	test/cpp/typedarrays.cpp \
	test/cpp/weak.cpp \
	test/cpp/weak2.cpp \
	test/cpp/wrappedobjectfactory.cpp \
	node_modules/node-gyp/gyp/data/win/large-pdb-shim.cc

FILTER = -build/include_subdir,-whitespace/parens

.PHONY: lint

lint:
	cd $(TOPLEVEL) && $(PYTHON) $(CPPLINT) --filter=$(FILTER) $(LINT_SOURCES)

test: $(ADDONS)
	npm test

forcetest:
	cd test && ../node_modules/.bin/node-gyp rebuild && cd ..
	npm test

docs: README.md doc/.build.sh doc/asyncworker.md doc/buffers.md doc/callback.md \
		doc/converters.md doc/errors.md doc/maybe_types.md doc/methods.md doc/new.md \
		doc/node_misc.md doc/persistent.md doc/scopes.md doc/script.md doc/string_bytes.md \
		doc/v8_internals.md doc/json.md doc/v8_misc.md
	doc/.build.sh


$(ADDONS): nan.h nan_new.h nan_implementation_pre_12_inl.h nan_implementation_12_inl.h \
		nan_callbacks.h nan_callbacks_12_inl.h nan_callbacks_pre_12_inl.h \
		nan_converters.h nan_converters_43_inl.h nan_converters_pre_43_inl.h \
		nan_define_own_property_helper.h \
		nan_json.h nan_maybe_43_inl.h nan_maybe_pre_43_inl.h \
		nan_persistent_12_inl.h nan_persistent_pre_12_inl.h nan_private.h \
		nan_weak.h nan_scriptorigin.h nan_string_bytes.h \
		test/binding.gyp $(SOURCES)
	cd test/ && ../node_modules/.bin/node-gyp rebuild
