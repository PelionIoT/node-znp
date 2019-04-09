{
  "targets": [
    {
      "target_name": "znp",
      "sources": [
        "./src/znp.cc",
        "./deps/znp-host-framework/examples/zclSendRcv/zclSendRcv.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl_gateway.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/znp_mngt.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl/zcl_general.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl/zcl_lighting.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl/zcl_hvac.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl/zcl.c",
        "./deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl_port/zcl_port.c",
        "./deps/znp-host-framework/framework/rpc/rpc.c",
        "./deps/znp-host-framework/framework/rpc/queue.c",
        "./deps/znp-host-framework/framework/mt/mtParser.c",
        "./deps/znp-host-framework/framework/mt/Zdo/mtZdo.c",
        "./deps/znp-host-framework/framework/mt/Sys/mtSys.c",
        "./deps/znp-host-framework/framework/mt/Sapi/mtSapi.c",
        "./deps/znp-host-framework/framework/mt/Af/mtAf.c",
        "./deps/znp-host-framework/framework/platform/gnu/dbgPrint.c",
        "./deps/znp-host-framework/framework/platform/gnu/hostConsole.c",
        "./deps/znp-host-framework/framework/platform/gnu/rpcTransport.c"
      ],
      "include_dirs": [
        "src/",
        "deps/znp-host-framework/framework/mt",
        "deps/znp-host-framework/framework/mt/Af",
        "deps/znp-host-framework/framework/mt/Sapi",
        "deps/znp-host-framework/framework/mt/Sys",
        "deps/znp-host-framework/framework/mt/Zdo",
        "deps/znp-host-framework/framework/platform/gnu",
        "deps/znp-host-framework/framework/rpc",
        "deps/znp-host-framework/examples/zclSendRcv",
        "deps/znp-host-framework/examples/zclSendRcv/zigbeeHa",
        "deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl",
        "deps/znp-host-framework/examples/zclSendRcv/zigbeeHa/zcl_port",
        "<!(node -e \"require('nan')\")"
      ],
      "configurations": {
        "Release": {
          "cflags": [ 
            "-fPIC",
            "-ggdb",
            "-Wall", 
            "-std=c++11",
            "-DxCC26xx", 
            "-DZCL_LEVEL_CTRL", 
            "-DZCL_HVAC_CLUSTER",
            "-DZCL_ON_OFF", 
            "-DZCL_READ", 
            "-DZCL_WRITE", 
            "-DZCL_STANDALONE",
            "-Wno-ignored-qualifiers"
          ],
          "xcode_settings": {
            "OTHER_CFLAGS": [
              "-Wno-ignored-qualifiers"
            ]
          }
        }
      }
    }
  ]
}
