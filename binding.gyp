{
  "targets": [
    {
      "target_name": "tod",
      "sources": [ "src/module.cpp" ],
	  'include_dirs': [ 'src', 'src/rtaudio'],
	  'conditions': [
        ['OS=="mac"',
          {
            'defines': ['__MACOSX_CORE__'],
            'libraries': ['-framework CoreAudio', '-framework CoreFoundation','-lpthread'],
            'include_dirs': [],
            'library_dirs': [],
            'sources': ['src/RtAudio.cpp'],
            'cflags':["-fexceptions", "-Wno-unused-but-set-variable","-Wno-unused-parameter","-Wno-unused-variable"],
            'cflags_cc':["-fexceptions", "-Wno-unused-but-set-variable","-Wno-unused-parameter","-Wno-unused-variable"],
            'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'},
          }
        ],
        ['OS=="linux"', {
          'libraries': []
          }
        ],
        ['OS=="win"',
          {
            'include_dirs': [ "./src/rtaudio"],
            'library_dirs': [ "./lib/windows" ],
            'libraries': [ 
              "freenect2.lib",
              "libusb-1.0.lib",
              "rtaudio.lib", 
              "openvr_api.lib",
              "winmm.lib", 
              "ole32.lib"
              ],
            'defines' : [
              '__WINDOWS_ASIO__',
              'RTAUDIO_EXPORT',
              'WIN32_LEAN_AND_MEAN',
              'VC_EXTRALEAN'
            ],
            'sources': [
              
            ],
            'msvs_settings' : {
              'VCCLCompilerTool' : {
                'AdditionalOptions' : ['/O2','/Oy','/GL','/GF','/Gm-','/EHsc','/MT','/GS','/Gy','/GR-','/Gd']
              },
              'VCLinkerTool' : {
                'AdditionalOptions' : ['/OPT:REF','/OPT:ICF','/LTCG']
              },
            },
            'copies': [
              {
                'destination': './build/Release/',
                'files': [
                  "./lib/windows/rtaudio.dll",
                  "./lib/windows/freenect2.dll",
                  "./lib/windows/libusb-1.0.dll",
                  "./lib/windows/openvr_api.dll"
                ]
              }
            ],
          }
        ],
      ],
    }
  ]
}