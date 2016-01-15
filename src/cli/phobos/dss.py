#!/usr/bin/python

# Copyright CEA/DAM 2015
# This file is part of the Phobos project

"""
High level, object-oriented interface over DSS. This is the module to use
to interact with phobos DSS layer, as it provides a safe, expressive and
clean API to access it.
"""

import os.path
from socket import gethostname

import phobos.capi.dss as cdss
import phobos.capi.ldm as cldm


# Valid filter suffix and associated operators.
# No suffix means DSS_CMP_EQ.
FILTER_OPERATORS = (
    ('__not', cdss.DSS_CMP_NE),
    ('__gt', cdss.DSS_CMP_GT),
    ('__ge', cdss.DSS_CMP_GE),
    ('__lt', cdss.DSS_CMP_LT),
    ('__le', cdss.DSS_CMP_LE),
    ('__like', cdss.DSS_CMP_LIKE),
    ('__jcontain', cdss.DSS_CMP_JSON_CTN),
    ('__jexist', cdss.DSS_CMP_JSON_EXIST)
)

OBJECT_PREFIXES = {
    'device': 'DSS_DEV',
    'extent': 'DSS_EXT',
    'media':  'DSS_MDA',
}


class GenericError(BaseException):
    """Base error to described DSS failures."""

def key_convert(obj_type, key):
    """Split key, return actual name and associated DSS_CMP_* operator."""
    kname = key
    comp = cdss.DSS_CMP_EQ # default
    kname_prefx = OBJECT_PREFIXES[obj_type] # KeyError on unsupported obj_type
    for sufx, comp_enum in FILTER_OPERATORS:
        if key.endswith(sufx):
            kname, comp = key[:len(sufx):], comp_enum
            break
    return getattr(cdss, '%s_%s' % (kname_prefx, kname)), comp


def dss_filter(obj_type, **kwargs):
    """Convert a k/v filter into a CDSS-compatible list of criteria."""
    filt = []
    for key, val in kwargs.iteritems():
        key, comp = key_convert(obj_type, key)
        crit = cdss.dss_crit()
        crit.crit_name = key
        crit.crit_cmp = comp
        crit.crit_val = cdss.dss_val()
        cdss.str2dss_val_fill(key, str(val), crit.crit_val)
        filt.append(crit)
    return filt

class ObjectManager(object):
    """Proxy to manipulate (CRUD) objects in DSS."""
    def __init__(self, obj_type, client, **kwargs):
        """Initialize new instance."""
        super(ObjectManager, self).__init__(**kwargs)
        self.obj_type = obj_type
        self.client = client

    def get(self, **kwargs):
        """Retrieve objects from DSS."""
        method = getattr(cdss, 'dss_%s_get' % self.obj_type)
        return method(self.client.handle, dss_filter(self.obj_type, **kwargs))

    def insert(self, objects):
        """Insert objects into DSS"""
        method = getattr(cdss, 'dss_%s_set' % self.obj_type)
        return method(self.client.handle, objects, cdss.DSS_SET_INSERT)

    def update(self, objects):
        """Update objects in DSS"""
        method = getattr(cdss, 'dss_%s_set' % self.obj_type)
        return method(self.client.handle, objects, cdss.DSS_SET_UPDATE)

    def delete(self, objects):
        """Delete objects from DSS"""
        method = getattr(cdss, 'dss_%s_set' % self.obj_type)
        return method(self.client.handle, objects, cdss.DSS_SET_DELETE)

class DeviceManager(ObjectManager):
    """Proxy to manipulate devices."""
    def add(self, device_type, device_path, locked=False):
        """Query device and insert information into DSS."""
        real_path = os.path.realpath(device_path)

        # We could use swig typemaps tricks to return a dev_adapter directly
        # from cldm.get_dev_adapter() but this is simpler and only done here.
        dev_adapter = cldm.dev_adapter()
        rc = cldm.get_dev_adapter(device_type, dev_adapter)
        if rc:
            raise GenericError("Cannot get device adpater")

        # Idem typemaps.
        dev_state = cldm.ldm_dev_state()
        rc = cldm.ldm_dev_query(dev_adapter, real_path, dev_state)
        if rc:
            raise GenericError("Cannot query device '%s'" % device_path)

        dev_info = cdss.dev_info()
        dev_info.family = dev_state.lds_family
        dev_info.model = dev_state.lds_model
        dev_info.path = device_path
        dev_info.host = gethostname().split('.')[0]
        dev_info.serial = dev_state.lds_serial
        if locked:
            dev_info.adm_status = cdss.PHO_DEV_ADM_ST_LOCKED
        else:
            dev_info.adm_status = cdss.PHO_DEV_ADM_ST_UNLOCKED

        return self.insert([dev_info])

class Client(object):
    """Main class: issue requests to the DSS and format replies."""
    def __init__(self, **kwargs):
        """Initialize a new DSS context."""
        super(Client, self).__init__(**kwargs)
        self.handle = None
        self.devices = DeviceManager('device', self)
        self.extents = ObjectManager('extent', self)
        self.objects = ObjectManager('object', self)
        # TODO consider adding a MediaManager with an add() method as well
        self.media = ObjectManager('media', self)

    def connect(self, **kwargs):
        """
        Establish a fresh connection or renew a stalled one if needed.

        A special '_connect' keyword is supported, that overrides everything
        and whose value is used as is to initialize connection. If not present,
        a connection string is crafted: 'k0=v0 k1=v1...'
        """
        if self.handle is not None:
            self.disconnect()

        # Build a string of the type 'dbname=phobos user=blah password=foobar'
        conn_info = kwargs.get('_connect')
        if conn_info is None:
            conn_info = ' '.join(['%s=%s' % (k, v) for k, v in kwargs.items()])

        self.handle = cdss.dss_handle()
        rcode = cdss.dss_init(conn_info, self.handle)
        if rcode != 0:
            raise GenericError('DSS initialization failed')

    def disconnect(self):
        """Disconnect from DSS and reset handle."""
        if self.handle is not None:
            cdss.dss_fini(self.handle)
            self.handle = None
