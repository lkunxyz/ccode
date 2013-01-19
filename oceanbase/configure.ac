    AC_INIT([OceanBase],
            [0.2.1],
            [qushan@taobao.com],
            [oceanbase])
    
    AC_PROG_LIBTOOL
    AM_INIT_AUTOMAKE(oceanbase, 0.2.1) 
    ac_default_prefix=${HOME}/oceanbase_bin# for convenience
    AC_PROG_CXX
    AC_LANG([C++])
    AC_ARG_WITH([tblib-root],
           AS_HELP_STRING([--with-tblib-root],
           [tbnet installation path]),
           TBLIB_ROOT=${withval})

    if  ! test -d "${TBLIB_ROOT}/include"; then
       AC_MSG_ERROR([TBLIB_ROOT have not been set])
       exit 1;
    fi
    dnl tbnet include path supplied
    AC_SUBST([TBLIB_ROOT])

    AC_ARG_WITH([release],
           AS_HELP_STRING([--with-release],
                           [use optimize (default is NO)]),
                           [
                            if test "$withval" = "yes"; then
                                    CXXFLAGS="-g -O2 -D_NO_EXCEPTION -DCOMPATIBLE -finline-functions -D__STDC_LIMIT_MACROS \
				              -Wall -Werror -Wextra -Wunused-parameter -Wformat -fno-strict-aliasing \
					      -Wconversion -Wdeprecated -D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64 \
					      -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE"
                            fi
                            ],
                            [ CXXFLAGS="-D__STDC_LIMIT_MACROS -D_NO_EXCEPTION  -DCOMPATIBLE -Wall -Werror -Wextra -Wunused-parameter \
			    		-Wformat -Wconversion -Wdeprecated -g"]
                            )
    AC_ARG_WITH([test_case],
           AS_HELP_STRING([--with-test-case],
		          [with test case (default is YES)]),
	   		  [
			    if test "$withval" = "no"; then
			    	test_case=no
			    fi
			  ],
			  [ test_case=yes ]
	   )

    AC_ARG_WITH([coverage],
           AS_HELP_STRING([--with-coverage],
		          [with coverage (default is NO)]),
	   		  [
			    if test "$withval" = "yes"; then
			    	coverage=yes
			    fi
			  ],
			  [coverage=no]
	   )

	AC_ARG_VAR([RELEASEID], [release number when building in Taobao ABS])
	AC_DEFINE_UNQUOTED([RELEASEID], "$RELEASEID")

    AM_CONDITIONAL([WITH_TEST_CASE], test x$test_case = xyes )

    AM_CONDITIONAL([COVERAGE], test x$coverage = xyes )

    dnl see whether subversion is installed
    AC_PATH_PROG(svnversioncommand, svnversion)

    dnl use svnversion to record the current repository revision only if
    dnl subversion is installed and we are in a working copy
    if test "X$svnversioncommand" = "X" || test `$svnversioncommand -n '.'` = "exported"; then
        mysvnversion="cat \$(top_srcdir)/svn_dist_version"
        AM_CONDITIONAL([HAVESVNWC], false)
    else
        mysvnversion="svnversion -n '.'"
        AM_CONDITIONAL([HAVESVNWC], true)
    fi

    AC_SUBST(mysvnversion)
    
    AC_CONFIG_FILES([\
                     Makefile \
                     tools/Makefile \
                     tools/syschecker/Makefile \
                     tools/syschecker/client/Makefile \
                     tools/dump_data/Makefile \
                     tools/dump_data/test/Makefile \
                     tools/obsql/Makefile \
                     src/Makefile \
                     src/common/Makefile \
                     src/common/compress/Makefile \
                     src/common/btree/Makefile \
                     src/common/btree/test/Makefile \
                     src/rootserver/Makefile \
                     src/chunkserver/Makefile \
                     src/updateserver/Makefile \
                     src/mergeserver/Makefile \
                     src/sstable/Makefile \
                     src/lsync/Makefile \
                     tests/Makefile \
                     tests/chunkserver/Makefile \
                     tests/common/Makefile \
                     tests/common/compress/Makefile \
                     tests/common/hash/Makefile \
                     tests/rootserver/Makefile \
                     tests/updateserver/Makefile \
                     tests/updateserver/mixed_test/Makefile \
                     tests/updateserver/stress_test/Makefile \
                     tests/mergeserver/Makefile \
                     tests/sstable/Makefile \
                     tests/lsync/Makefile \
                     ])
    AC_OUTPUT
