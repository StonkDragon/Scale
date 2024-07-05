#define _SCL_PREPROC_NARG(...) \
         _SCL_PREPROC_NARG_(__VA_ARGS__,_SCL_PREPROC_RSEQ_N())
#define _SCL_PREPROC_NARG_(...) \
         _SCL_PREPROC_ARG_N(__VA_ARGS__)
#define _SCL_PREPROC_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define _SCL_PREPROC_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define _SCL_TYPES_0(X, ...)
#define _SCL_TYPES_1(X, ...) typeof(X)
#define _SCL_TYPES_2(X, ...) typeof(X), _SCL_TYPES_1(__VA_ARGS__)
#define _SCL_TYPES_3(X, ...) typeof(X), _SCL_TYPES_2(__VA_ARGS__)
#define _SCL_TYPES_4(X, ...) typeof(X), _SCL_TYPES_3(__VA_ARGS__)
#define _SCL_TYPES_5(X, ...) typeof(X), _SCL_TYPES_4(__VA_ARGS__)
#define _SCL_TYPES_6(X, ...) typeof(X), _SCL_TYPES_5(__VA_ARGS__)
#define _SCL_TYPES_7(X, ...) typeof(X), _SCL_TYPES_6(__VA_ARGS__)
#define _SCL_TYPES_8(X, ...) typeof(X), _SCL_TYPES_7(__VA_ARGS__)
#define _SCL_TYPES_9(X, ...) typeof(X), _SCL_TYPES_8(__VA_ARGS__)
#define _SCL_TYPES_10(X, ...) typeof(X), _SCL_TYPES_9(__VA_ARGS__)
#define _SCL_TYPES_11(X, ...) typeof(X), _SCL_TYPES_10(__VA_ARGS__)
#define _SCL_TYPES_12(X, ...) typeof(X), _SCL_TYPES_11(__VA_ARGS__)
#define _SCL_TYPES_13(X, ...) typeof(X), _SCL_TYPES_12(__VA_ARGS__)
#define _SCL_TYPES_14(X, ...) typeof(X), _SCL_TYPES_13(__VA_ARGS__)
#define _SCL_TYPES_15(X, ...) typeof(X), _SCL_TYPES_14(__VA_ARGS__)
#define _SCL_TYPES_16(X, ...) typeof(X), _SCL_TYPES_15(__VA_ARGS__)
#define _SCL_TYPES_17(X, ...) typeof(X), _SCL_TYPES_16(__VA_ARGS__)
#define _SCL_TYPES_18(X, ...) typeof(X), _SCL_TYPES_17(__VA_ARGS__)
#define _SCL_TYPES_19(X, ...) typeof(X), _SCL_TYPES_18(__VA_ARGS__)
#define _SCL_TYPES_20(X, ...) typeof(X), _SCL_TYPES_19(__VA_ARGS__)
#define _SCL_TYPES_21(X, ...) typeof(X), _SCL_TYPES_20(__VA_ARGS__)
#define _SCL_TYPES_22(X, ...) typeof(X), _SCL_TYPES_21(__VA_ARGS__)
#define _SCL_TYPES_23(X, ...) typeof(X), _SCL_TYPES_22(__VA_ARGS__)
#define _SCL_TYPES_24(X, ...) typeof(X), _SCL_TYPES_23(__VA_ARGS__)
#define _SCL_TYPES_25(X, ...) typeof(X), _SCL_TYPES_24(__VA_ARGS__)
#define _SCL_TYPES_26(X, ...) typeof(X), _SCL_TYPES_25(__VA_ARGS__)
#define _SCL_TYPES_27(X, ...) typeof(X), _SCL_TYPES_26(__VA_ARGS__)
#define _SCL_TYPES_28(X, ...) typeof(X), _SCL_TYPES_27(__VA_ARGS__)
#define _SCL_TYPES_29(X, ...) typeof(X), _SCL_TYPES_28(__VA_ARGS__)
#define _SCL_TYPES_30(X, ...) typeof(X), _SCL_TYPES_29(__VA_ARGS__)
#define _SCL_TYPES_31(X, ...) typeof(X), _SCL_TYPES_30(__VA_ARGS__)
#define _SCL_TYPES_32(X, ...) typeof(X), _SCL_TYPES_31(__VA_ARGS__)
#define _SCL_TYPES_33(X, ...) typeof(X), _SCL_TYPES_32(__VA_ARGS__)
#define _SCL_TYPES_34(X, ...) typeof(X), _SCL_TYPES_33(__VA_ARGS__)
#define _SCL_TYPES_35(X, ...) typeof(X), _SCL_TYPES_34(__VA_ARGS__)
#define _SCL_TYPES_36(X, ...) typeof(X), _SCL_TYPES_35(__VA_ARGS__)
#define _SCL_TYPES_37(X, ...) typeof(X), _SCL_TYPES_36(__VA_ARGS__)
#define _SCL_TYPES_38(X, ...) typeof(X), _SCL_TYPES_37(__VA_ARGS__)
#define _SCL_TYPES_39(X, ...) typeof(X), _SCL_TYPES_38(__VA_ARGS__)
#define _SCL_TYPES_40(X, ...) typeof(X), _SCL_TYPES_39(__VA_ARGS__)
#define _SCL_TYPES_41(X, ...) typeof(X), _SCL_TYPES_40(__VA_ARGS__)
#define _SCL_TYPES_42(X, ...) typeof(X), _SCL_TYPES_41(__VA_ARGS__)
#define _SCL_TYPES_43(X, ...) typeof(X), _SCL_TYPES_42(__VA_ARGS__)
#define _SCL_TYPES_44(X, ...) typeof(X), _SCL_TYPES_43(__VA_ARGS__)
#define _SCL_TYPES_45(X, ...) typeof(X), _SCL_TYPES_44(__VA_ARGS__)
#define _SCL_TYPES_46(X, ...) typeof(X), _SCL_TYPES_45(__VA_ARGS__)
#define _SCL_TYPES_47(X, ...) typeof(X), _SCL_TYPES_46(__VA_ARGS__)
#define _SCL_TYPES_48(X, ...) typeof(X), _SCL_TYPES_47(__VA_ARGS__)
#define _SCL_TYPES_49(X, ...) typeof(X), _SCL_TYPES_48(__VA_ARGS__)
#define _SCL_TYPES_50(X, ...) typeof(X), _SCL_TYPES_49(__VA_ARGS__)
#define _SCL_TYPES_51(X, ...) typeof(X), _SCL_TYPES_50(__VA_ARGS__)
#define _SCL_TYPES_52(X, ...) typeof(X), _SCL_TYPES_51(__VA_ARGS__)
#define _SCL_TYPES_53(X, ...) typeof(X), _SCL_TYPES_52(__VA_ARGS__)
#define _SCL_TYPES_54(X, ...) typeof(X), _SCL_TYPES_53(__VA_ARGS__)
#define _SCL_TYPES_55(X, ...) typeof(X), _SCL_TYPES_54(__VA_ARGS__)
#define _SCL_TYPES_56(X, ...) typeof(X), _SCL_TYPES_55(__VA_ARGS__)
#define _SCL_TYPES_57(X, ...) typeof(X), _SCL_TYPES_56(__VA_ARGS__)
#define _SCL_TYPES_58(X, ...) typeof(X), _SCL_TYPES_57(__VA_ARGS__)
#define _SCL_TYPES_59(X, ...) typeof(X), _SCL_TYPES_58(__VA_ARGS__)
#define _SCL_TYPES_60(X, ...) typeof(X), _SCL_TYPES_59(__VA_ARGS__)
#define _SCL_TYPES_61(X, ...) typeof(X), _SCL_TYPES_60(__VA_ARGS__)
#define _SCL_TYPES_62(X, ...) typeof(X), _SCL_TYPES_61(__VA_ARGS__)
#define _SCL_TYPES_63(X, ...) typeof(X), _SCL_TYPES_62(__VA_ARGS__)
#define _SCL_TYPES_64(X, ...) typeof(X), _SCL_TYPES_63(__VA_ARGS__)

#define _SCL_TYPES__(n, ...) _SCL_TYPES_ ## n (__VA_ARGS__)
#define _SCL_TYPES_(n, ...) _SCL_TYPES__(n, __VA_ARGS__)
#define _SCL_TYPES(...) _SCL_TYPES_(_SCL_PREPROC_NARG(__VA_ARGS__), ## __VA_ARGS__)

#define _SCL_PREPROC_REPEAT_N(n, what) _SCL_PREPROC_REPEAT_N_(n, what)
#define _SCL_PREPROC_REPEAT_N_(n, what) _SCL_PREPROC_REPEAT_ ## n(what)
#define _SCL_PREPROC_REPEAT_0(what)
#define _SCL_PREPROC_REPEAT_1(what) what
#define _SCL_PREPROC_REPEAT_2(what) _SCL_PREPROC_REPEAT_1(what), what
#define _SCL_PREPROC_REPEAT_3(what) _SCL_PREPROC_REPEAT_2(what), what
#define _SCL_PREPROC_REPEAT_4(what) _SCL_PREPROC_REPEAT_3(what), what
#define _SCL_PREPROC_REPEAT_5(what) _SCL_PREPROC_REPEAT_4(what), what
#define _SCL_PREPROC_REPEAT_6(what) _SCL_PREPROC_REPEAT_5(what), what
#define _SCL_PREPROC_REPEAT_7(what) _SCL_PREPROC_REPEAT_6(what), what
#define _SCL_PREPROC_REPEAT_8(what) _SCL_PREPROC_REPEAT_7(what), what
#define _SCL_PREPROC_REPEAT_9(what) _SCL_PREPROC_REPEAT_8(what), what
#define _SCL_PREPROC_REPEAT_10(what) _SCL_PREPROC_REPEAT_9(what), what
#define _SCL_PREPROC_REPEAT_11(what) _SCL_PREPROC_REPEAT_10(what), what
#define _SCL_PREPROC_REPEAT_12(what) _SCL_PREPROC_REPEAT_11(what), what
#define _SCL_PREPROC_REPEAT_13(what) _SCL_PREPROC_REPEAT_12(what), what
#define _SCL_PREPROC_REPEAT_14(what) _SCL_PREPROC_REPEAT_13(what), what
#define _SCL_PREPROC_REPEAT_15(what) _SCL_PREPROC_REPEAT_14(what), what
#define _SCL_PREPROC_REPEAT_16(what) _SCL_PREPROC_REPEAT_15(what), what
#define _SCL_PREPROC_REPEAT_17(what) _SCL_PREPROC_REPEAT_16(what), what
#define _SCL_PREPROC_REPEAT_18(what) _SCL_PREPROC_REPEAT_17(what), what
#define _SCL_PREPROC_REPEAT_19(what) _SCL_PREPROC_REPEAT_18(what), what
#define _SCL_PREPROC_REPEAT_20(what) _SCL_PREPROC_REPEAT_19(what), what
#define _SCL_PREPROC_REPEAT_21(what) _SCL_PREPROC_REPEAT_20(what), what
#define _SCL_PREPROC_REPEAT_22(what) _SCL_PREPROC_REPEAT_21(what), what
#define _SCL_PREPROC_REPEAT_23(what) _SCL_PREPROC_REPEAT_22(what), what
#define _SCL_PREPROC_REPEAT_24(what) _SCL_PREPROC_REPEAT_23(what), what
#define _SCL_PREPROC_REPEAT_25(what) _SCL_PREPROC_REPEAT_24(what), what
#define _SCL_PREPROC_REPEAT_26(what) _SCL_PREPROC_REPEAT_25(what), what
#define _SCL_PREPROC_REPEAT_27(what) _SCL_PREPROC_REPEAT_26(what), what
#define _SCL_PREPROC_REPEAT_28(what) _SCL_PREPROC_REPEAT_27(what), what
#define _SCL_PREPROC_REPEAT_29(what) _SCL_PREPROC_REPEAT_28(what), what
#define _SCL_PREPROC_REPEAT_30(what) _SCL_PREPROC_REPEAT_29(what), what
#define _SCL_PREPROC_REPEAT_31(what) _SCL_PREPROC_REPEAT_30(what), what
#define _SCL_PREPROC_REPEAT_32(what) _SCL_PREPROC_REPEAT_31(what), what
#define _SCL_PREPROC_REPEAT_33(what) _SCL_PREPROC_REPEAT_32(what), what
#define _SCL_PREPROC_REPEAT_34(what) _SCL_PREPROC_REPEAT_33(what), what
#define _SCL_PREPROC_REPEAT_35(what) _SCL_PREPROC_REPEAT_34(what), what
#define _SCL_PREPROC_REPEAT_36(what) _SCL_PREPROC_REPEAT_35(what), what
#define _SCL_PREPROC_REPEAT_37(what) _SCL_PREPROC_REPEAT_36(what), what
#define _SCL_PREPROC_REPEAT_38(what) _SCL_PREPROC_REPEAT_37(what), what
#define _SCL_PREPROC_REPEAT_39(what) _SCL_PREPROC_REPEAT_38(what), what
#define _SCL_PREPROC_REPEAT_40(what) _SCL_PREPROC_REPEAT_39(what), what
#define _SCL_PREPROC_REPEAT_41(what) _SCL_PREPROC_REPEAT_40(what), what
#define _SCL_PREPROC_REPEAT_42(what) _SCL_PREPROC_REPEAT_41(what), what
#define _SCL_PREPROC_REPEAT_43(what) _SCL_PREPROC_REPEAT_42(what), what
#define _SCL_PREPROC_REPEAT_44(what) _SCL_PREPROC_REPEAT_43(what), what
#define _SCL_PREPROC_REPEAT_45(what) _SCL_PREPROC_REPEAT_44(what), what
#define _SCL_PREPROC_REPEAT_46(what) _SCL_PREPROC_REPEAT_45(what), what
#define _SCL_PREPROC_REPEAT_47(what) _SCL_PREPROC_REPEAT_46(what), what
#define _SCL_PREPROC_REPEAT_48(what) _SCL_PREPROC_REPEAT_47(what), what
#define _SCL_PREPROC_REPEAT_49(what) _SCL_PREPROC_REPEAT_48(what), what
#define _SCL_PREPROC_REPEAT_50(what) _SCL_PREPROC_REPEAT_49(what), what
#define _SCL_PREPROC_REPEAT_51(what) _SCL_PREPROC_REPEAT_50(what), what
#define _SCL_PREPROC_REPEAT_52(what) _SCL_PREPROC_REPEAT_51(what), what
#define _SCL_PREPROC_REPEAT_53(what) _SCL_PREPROC_REPEAT_52(what), what
#define _SCL_PREPROC_REPEAT_54(what) _SCL_PREPROC_REPEAT_53(what), what
#define _SCL_PREPROC_REPEAT_55(what) _SCL_PREPROC_REPEAT_54(what), what
#define _SCL_PREPROC_REPEAT_56(what) _SCL_PREPROC_REPEAT_55(what), what
#define _SCL_PREPROC_REPEAT_57(what) _SCL_PREPROC_REPEAT_56(what), what
#define _SCL_PREPROC_REPEAT_58(what) _SCL_PREPROC_REPEAT_57(what), what
#define _SCL_PREPROC_REPEAT_59(what) _SCL_PREPROC_REPEAT_58(what), what
#define _SCL_PREPROC_REPEAT_60(what) _SCL_PREPROC_REPEAT_59(what), what
#define _SCL_PREPROC_REPEAT_61(what) _SCL_PREPROC_REPEAT_60(what), what
#define _SCL_PREPROC_REPEAT_62(what) _SCL_PREPROC_REPEAT_61(what), what
#define _SCL_PREPROC_REPEAT_63(what) _SCL_PREPROC_REPEAT_62(what), what
#define _SCL_PREPROC_REPEAT_64(what) _SCL_PREPROC_REPEAT_63(what), what
