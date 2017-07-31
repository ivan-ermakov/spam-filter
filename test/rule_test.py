import os.path
import ctypes

libsf = ctypes.CDLL(os.path.dirname(os.path.abspath(__file__)) + "/../bin/libsf.so.1.0")

libsf.rule_init.argtypes = [ctypes.c_char_p]
libsf.rule_init.restype = ctypes.c_void_p
libsf.rule_get_rpn.argtypes = [ctypes.c_void_p] 
libsf.rule_get_rpn.restype = ctypes.c_char_p

rule = libsf.rule_init("0 bool 1 (1 && 2)")
rpn = libsf.rule_get_rpn(rule)
print (rpn)

if rpn == "1 2 && ":
    print ("Result: OK")
elif msg_type == MSG_TYPE_SPAM:
	print ("Result: Error. Not expected answer from library %d" % msg_type.value)