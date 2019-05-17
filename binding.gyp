{
  "targets": [
    {
      "target_name": "tod",
      "sources": [ "src/module.cpp", 'src/RtAudio.cpp' ],
	  'include_dirs': [ 'src', 'src/rtaudio' ],
	  'conditions': [
        ['OS=="mac"',
          {
            'defines': ['__MACOSX_CORE__'],
            'libraries': ['-framework CoreAudio', '-framework CoreFoundation','-lpthread'],
            'include_dirs': [],
            'library_dirs': [],
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
            'include_dirs': [],
            'library_dirs': [],
            'libraries': [],
            'defines' : [
              'WIN32_LEAN_AND_MEAN',
              'VC_EXTRALEAN'
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
                'files': []
              }
            ],
          }
        ],
      ],
    }
  ]
}