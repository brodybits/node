'use strict';

const Thread = process.binding('thread_wrap').Thread;

exports.newThread = function(arg) {
  return new Thread();
};
