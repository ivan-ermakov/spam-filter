import os.path
import ctypes

libsf = ctypes.CDLL(os.path.dirname(os.path.abspath(__file__)) + "/../bin/libsf.so.1.0")

MSG_TYPE_HAM = 0
MSG_TYPE_SPAM = 1

libsf.spam_filter_init.argtypes = [ctypes.c_char_p]
libsf.spam_filter_init.restype = ctypes.c_void_p
libsf.spam_filter_check_msg.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p] 
libsf.spam_filter_check_msg.restype = ctypes.c_int

spam_filter = libsf.spam_filter_init("rules.txt")

msg_type = ctypes.c_int()
libsf.spam_filter_check_msg(spam_filter, "viagra", ctypes.byref(msg_type))

if msg_type.value == MSG_TYPE_HAM:
    print ("Result: Error. Expecting that message is spam")
elif msg_type.value == MSG_TYPE_SPAM:
    print ("Result: OK")
else:
   print ("Result: Error. Not expected answer from library %d" % msg_type.value)