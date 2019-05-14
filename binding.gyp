{
  "targets": [
    {
      "target_name": "tod",
      "sources": [ "src/module.cpp" ],
	  'include_dirs': [ 'src' ],
	  'conditions': [
        ['OS=="mac"',
          {
            'libraries': [],
            'include_dirs': [],
            'library_dirs': [],
            'cflags':["-Wno-unused-but-set-variable","-Wno-unused-parameter","-Wno-unused-variable"]
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