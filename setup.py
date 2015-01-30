#!/usr/bin/env python
from setuptools import setup

setup(
    name='trezor',
    version='0.6.1',
    author='Bitcoin TREZOR',
    author_email='info@bitcointrezor.com',
    description='Python library for communicating with TREZOR Bitcoin Hardware Wallet',
    url='https://github.com/trezor/python-trezor',
    py_modules=[
        'trezorlib.ckd_public',
        'trezorlib.client',
        'trezorlib.debuglink',
        'trezorlib.mapping',
        'trezorlib.messages_pb2',
        'trezorlib.protobuf_json',
        'trezorlib.qt.pinmatrix',
        'trezorlib.tools',
        'trezorlib.transport',
        'trezorlib.transport_fake',
        'trezorlib.transport_hid',
        'trezorlib.transport_pipe',
        'trezorlib.transport_serial',
        'trezorlib.transport_socket',
        'trezorlib.tx_api',
        'trezorlib.types_pb2',
    ],
    test_suite='tests',
    install_requires=['ecdsa>=0.9', 'protobuf==2.5.0', 'mnemonic>=0.8', 'hidapi>=0.7.99'],
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        'License :: OSI Approved :: GNU Lesser General Public License v3 (LGPLv3)',
        'Operating System :: POSIX :: Linux',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: MacOS :: MacOS X',
    ],
)
