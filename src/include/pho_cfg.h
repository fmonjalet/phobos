/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
/*
 * Copyright 2014-2015 CEA/DAM. All Rights Reserved.
 */
/**
 * \brief  Phobos configuration management.
 *
 * For more details see doc/design/config.txt.
 */
#ifndef _PHO_CFG_H
#define _PHO_CFG_H

#include <sys/types.h>
#include <attr/xattr.h>

/** prefix string for environment variables */
#define PHO_ENV_PREFIX "PHOBOS"

/** default path to local config file */
#define PHO_DEFAULT_CFG "/etc/phobos.conf"

struct pho_config_item {
    char *section;
    char *name;
    char *value;
};

/**
 * Initialize access to local config parameters (process-wide and host-wide).
 * This is basically called before the DSS is initialized.
 * This is NOT thread safe and must be called before any call to other
 * pho_cfg_*() * functions.
 * @param[in] cfg_file force path to configuration file.
 *                     If cfg_file is NULL, get env(PHOBOS_CFG_FILE).
 *                     If this last is NULL, use the default path
 *                     ('/etc/phobos.conf').

 */
int pho_cfg_init_local(const char *config_file);

/** This function gets the value of a configuration item
 *  and return default value (from module_params) if it is not found.
 *  @return A the value on success and NULL on error.
 */
const char *_pho_cfg_get(int first_index, int last_index, int param_index,
                        const struct pho_config_item *module_params);

#define PHO_CFG_GET(_params_list, _cfg_namespace, _name)                \
        _pho_cfg_get(_cfg_namespace ## _FIRST, _cfg_namespace ## _LAST, \
                _cfg_namespace ## _ ##_name, (_params_list))

# ifndef SWIG
/**
 * Allow access to global config parameters for the current thread.
 * This can only be called after the DSS is initialized.
 */
int pho_cfg_set_thread_conn(void *dss_handle);

/**
 * This function gets the value of the configuration item
 * with the given name in the given section.
 *
 * @param(in) section   Name of the section to look for the parameter.
 * @param(in) name      Name of the parameter to read.
 * @param(out) value    Value of the parameter (const string, must not be
 *                      altered).
 *
 * @retval  0           The parameter is returned successfully.
 * @retval  -ENOATTR    The parameter is not found.
 */
int pho_cfg_get_val(const char *section, const char *name,
                    const char **value);

/**
 * This function gets the value of multiple configuration items
 * that match the given section_pattern and/or name_pattern.
 * String matching is shell-like (fnmatch(3)).
 *
 * @param(in) section_pattern   Pattern of the sections to look for name
 *                              patterns.
 *                              If NULL, search name_pattern is all sections.
 * @param(in) name_pattern      Pattern of parameter name to loook for.
 *                              If NULL, return all names in matching sections.
 * @param(out) items            List of matching configuration items.
 *                              The list is allocated by the call and must be
 *                              freed by the caller.
 * @param(out) count            Number of elements in items.
 *
 * @return 0 on success. If no matching parameter is found, the function still
           returns 0 and count is set to 0.
 */
int pho_cfg_match(const char *section_pattern, const char *name_pattern,
                  struct pho_config_item *items, int *count);

enum pho_cfg_flags {
    PHO_CFG_SCOPE_PROCESS = (1 << 0), /**< set the parameter only for current
                                           process */
    PHO_CFG_SCOPE_LOCAL   = (1 << 1), /**< set the parameter for local host */
    PHO_CFG_SCOPE_GLOBAL  = (1 << 2), /**< set the parameter for all phobos
                                           hosts and instances */
};

/**
 * Set the value of a parameter.
 * @param(in) section   Name of the section where the parameter is located.
 * @param(in) name      Name of the parameter to set.
 * @param(in) value     Value of the parameter.
 * @param(in) flags     mask of OR'ed 'enum pho_cfg_flags'. In particular,
 *                      flags indicate the scope of the parameter
 *                      (see doc/design/config.txt).
 * @return 0 on success, a negative error code on failure (errno(3)).
 */
int pho_cfg_set(const char *section, const char *name, const char *value,
                enum pho_cfg_flags flags);


/**
 * Helper to get a numeric configuration parameter.
 * @param[in] param       Parameter to be retrieved.
 * @param[in] fail_value  Returned value if parsing fails.
 * @return parameter value, or fail_value on error.
 */
int _pho_cfg_get_int(int first_index, int last_index, int param_index,
                     const struct pho_config_item *module_params,
                     int fail_val);

#define PHO_CFG_GET_INT(_params_list, _cfg_namespace, _name, _fail_val)    \
        _pho_cfg_get_int(_cfg_namespace ## _FIRST, _cfg_namespace ## _LAST, \
                _cfg_namespace ## _ ##_name, (_params_list), (_fail_val))

# endif
#endif
