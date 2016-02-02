/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2016 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

const test     = require('tap').test
    , testRoot = require('path').resolve(__dirname, '..')
    , bindings = require('bindings')({ module_root: testRoot, bindings: 'asyncprogressworker' });

test('asyncprogressworker', function (t) {
  var worker = bindings.a
    , self = this;
  self.progressed = 0;
  worker(100, 5, function(i) {
    t.ok(i === self.progressed, 'got the progress updates #' + i);
    self.progressed++;
  }, function () {
    t.ok(self.progressed === 5, 'got all progress updates');
    t.end();
  })
});

test('asyncprogressnowait1x', function (t) {
  var worker = bindings.b
    , self = this;
  self.progressed = 0;
  worker(1, function(i) {
    self.progressed++;
  }, function () {
    t.ok(self.progressed === 1, 'one message - got ' + self.progressed);
    t.end();
  })
});

test('asyncprogressnowait2x', function (t) {
  var worker = bindings.b
    , self = this;
  self.progressed = 0;
  worker(2, function(i) {
    self.progressed++;
  }, function () {
    t.ok(self.progressed === 2, 'two messages - got ' + self.progressed);
    t.end();
  })
});

test('asyncprogressnowait100x', function (t) {
  var worker = bindings.b
    , self = this;
  self.progressed = 0;
  worker(100, function(i) {
    self.progressed++;
  }, function () {
    t.ok(self.progressed === 100, '100 messages - got ' + self.progressed);
    t.end();
  })
});
