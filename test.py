MSG_TYPE_HAM = 0
MSG_TYPE_SPAM = 1

import os.path
import ctypes

spam_filter_lib = ctypes.CDLL(os.path.dirname(os.path.abspath(__file__)) + "/bin/libsf.so.1.0")
spam_filter = spam_filter_lib.spam_filter_init("rules.txt")
spam_filter
msg_type = ctypes.c_int()
spam_filter_lib.spam_filter_check_msg(spam_filter, "viagra", ctypes.byref(msg_type))
if msg_type == MSG_TYPE_HAM:
    print ("Result: Error. Expecting that message is spam")
elif msg_type == MSG_TYPE_SPAM:
    print ("Result: OK")
else:
   print ("Result: Error. Not expected answer from library %d" % msg_type.value)