AM_CFLAGS= $(CC_OPT)

# autotools automatically generate a 'make check' target that:
# 1) build check_PROGRAMS
# 2) run TESTS and generate a summary
check_PROGRAMS=test_common test_attrs test_mapper test_log test_store \
	       test_dss test_cfg test_scsi test_scsi_scan test_lintape \
               test_ldm test_type_utils test_store_retry

# 1st step: low level tests
UNIT_TESTS=test_common test_attrs test_mapper test_log test_cfg test_lintape \
	   test_ldm test_type_utils
# 2nd step: integration tests
INTEG_TESTS=test_dss.sh test_scsi.sh test_layouts.sh acceptance.sh \
		test_store_retry.sh

TESTS=$(UNIT_TESTS) $(INTEG_TESTS)

noinst_PROGRAMS=$(check_PROGRAMS)
noinst_HEADERS=pho_test_utils.h

pre-check-local:
	./test_lock.sh

check: pre-check-local check-am
	rm -f /tmp/phobos_test.lock

test_log_SOURCES=test_log.c
test_log_LDADD=../common/libpho_common.la

test_common_SOURCES=test_common.c
test_common_LDADD=../common/libpho_common.la

test_attrs_SOURCES=test_attrs.c
test_attrs_LDADD=../common/libpho_common.la

test_mapper_SOURCES=test_mapper.c
test_mapper_LDADD=../store/libphobos_store.la

test_store_SOURCES=test_store.c
test_store_LDADD=../store/libphobos_store.la

test_dss_SOURCES=test_dss.c
test_dss_LDADD=../dss/libpho_dss.la ../cfg/libpho_cfg.la ../common/libpho_common.la

test_cfg_SOURCES=test_cfg.c
test_cfg_LDADD=../cfg/libpho_cfg.la ../common/libpho_common.la

test_scsi_SOURCES=test_scsi.c
test_scsi_LDADD=../ldm/libpho_ldm.la ../cfg/libpho_cfg.la ../common/libpho_common.la

test_scsi_scan_SOURCES=test_scsi_scan.c
test_scsi_scan_LDADD=../ldm/libpho_ldm.la ../cfg/libpho_cfg.la ../common/libpho_common.la

test_lintape_SOURCES=test_lintape.c
test_lintape_LDADD=../ldm/libpho_ldm.la ../cfg/libpho_cfg.la ../common/libpho_common.la

test_ldm_SOURCES=test_ldm.c
test_ldm_LDADD=../ldm/libpho_ldm.la ../cfg/libpho_cfg.la ../common/libpho_common.la

test_type_utils_SOURCES=test_type_utils.c
test_type_utils_LDADD=../common/libpho_common.la

test_store_retry_SOURCES=test_store_retry.c
test_store_retry_LDADD=../store/libphobos_store.la
