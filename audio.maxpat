{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 8,
			"minor" : 0,
			"revision" : 5,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 459.0, 584.0, 752.0, 359.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-27",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 27.0, 74.0, 72.0, 22.0 ],
					"text" : "loadmess 0"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-28",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patching_rect" : [ 27.0, 107.0, 85.0, 22.0 ],
					"text" : "count~ 0 2048"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-26",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "float", "bang" ],
					"patching_rect" : [ 54.0, 186.0, 189.0, 22.0 ],
					"text" : "buffer~ beetles 8 8 @samps 2048"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-25",
					"linecount" : 2,
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 54.0, 144.5, 147.0, 35.0 ],
					"text" : "bin2buf~ @buffer beetles @file beetles.bin"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-23",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "float", "bang" ],
					"patching_rect" : [ 420.0, 268.0, 162.0, 22.0 ],
					"text" : "buffer~ sched @samps 2048"
				}

			}
, 			{
				"box" : 				{
					"attr" : "beetle_modulation",
					"id" : "obj-22",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 317.0, 186.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"attr" : "beetle_frequency",
					"id" : "obj-21",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 304.0, 151.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"attr" : "beetle_filter",
					"id" : "obj-20",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 294.0, 121.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"attr" : "beetle_dur",
					"id" : "obj-19",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 280.0, 92.0, 150.0, 22.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-17",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 600.773621000000048, 251.988434000000012, 126.0, 22.0 ],
					"text" : "qmetro 100 @active 1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 13.0,
					"format" : 6,
					"id" : "obj-54",
					"maxclass" : "flonum",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 600.773621000000048, 317.0, 57.0, 23.0 ],
					"triangle" : 0,
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 13.0,
					"id" : "obj-56",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "", "int" ],
					"patching_rect" : [ 600.773621000000048, 282.0, 84.0, 23.0 ],
					"text" : "adstatus cpu"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-12",
					"maxclass" : "ezdac~",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 27.0, 295.0, 45.0, 45.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-10",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 355.0, 231.0, 99.0, 22.0 ],
					"text" : "full_source_code"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-2",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 3,
					"outlettype" : [ "signal", "signal", "signal" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 0,
							"revision" : 5,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "dsp.gen",
						"rect" : [ 359.0, 78.0, 1092.0, 938.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"id" : "obj-15",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 13.0, 69.0, 28.0, 22.0 ],
									"text" : "in 1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-20",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 235.0, 27.0, 81.0, 22.0 ],
									"text" : "buffer beetles"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-18",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 152.0, 27.0, 74.0, 22.0 ],
									"text" : "buffer sched"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-12",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 897.0, 421.0, 123.0, 22.0 ],
									"text" : "param beetle_dur 0.1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-11",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 897.0, 390.0, 128.0, 22.0 ],
									"text" : "param beetle_filter 0.4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-10",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 897.0, 361.0, 154.0, 22.0 ],
									"text" : "param beetle_modulation 4"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-8",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 897.0, 328.0, 171.0, 22.0 ],
									"text" : "param beetle_base_period 0.1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-3",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 897.0, 299.0, 168.0, 22.0 ],
									"text" : "param beetle_frequency 3000"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-7",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 863.0, 939.0, 35.0, 22.0 ],
									"text" : "out 3"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-33",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 966.166687000000024, 102.0, 59.0, 20.0 ],
									"text" : "playback"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-32",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 925.0, 169.0, 19.0, 22.0 ],
									"text" : "0"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-30",
									"maxclass" : "newobj",
									"numinlets" : 3,
									"numoutlets" : 0,
									"patching_rect" : [ 1002.0, 208.0, 66.0, 22.0 ],
									"text" : "poke ola 1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-29",
									"maxclass" : "newobj",
									"numinlets" : 3,
									"numoutlets" : 0,
									"patching_rect" : [ 925.0, 208.0, 66.0, 22.0 ],
									"text" : "poke ola 0"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-28",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 863.0, 102.0, 97.0, 22.0 ],
									"text" : "peek ola 2"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-19",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 902.0, 139.0, 37.0, 22.0 ],
									"text" : "out 2"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-13",
									"linecount" : 6,
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 863.0, 169.0, 59.0, 87.0 ],
									"text" : "zero out the ola buffer once we have read it"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-14",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 863.0, 24.0, 19.0, 22.0 ],
									"text" : "1"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 3,
									"numoutlets" : 3,
									"outlettype" : [ "", "", "" ],
									"patching_rect" : [ 863.0, 58.0, 51.0, 22.0 ],
									"text" : "counter"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 895.0, 24.0, 100.0, 22.0 ],
									"text" : "data ola 44100 2"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-4",
									"maxclass" : "newobj",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 863.0, 139.0, 37.0, 22.0 ],
									"text" : "out 1"
								}

							}
, 							{
								"box" : 								{
									"code" : "NUM_BEETLES = dim(beetles);\r\nbeetle_rpts = 3;\r\nbeetle_size = 0.014;\r\n\r\nolaindex = in2;\r\nb = in1;\r\nage = peek(beetles, b);\r\nif (age > 0) {\t\r\n\tbtime = peek(sched, b) - NUM_BEETLES/SAMPLERATE;\r\n\tif (btime < 0) {\r\n\t\t\r\n\t\tbeetle_energy, \r\n\t\tbeetle_period, \r\n\t\tbeetle_freq, \r\n\t\tbeetle_mod, \r\n\t\tbeetle_scale, \r\n\t\tbeetle_pos_x, \r\n\t\tbeetle_pos_z\r\n\t\t= peek(beetles, b, 1, channels=7);\n\r\n\t\t\r\n\t\t// adjust genes:\r\n\t\tbeetle_period = beetle_base_period * (beetle_period*5. + 3.);\r\n\t\tbeetle_freq = 0.5 + beetle_freq;\r\n\t\tbeetle_mod = (beetle_mod - 0.5) * beetle_modulation;\r\n\t\t\r\n\t\tperiod = beetle_period;\t// approx 0.1 .. 1 (seconds?)\r\n\t\tbtime += period * (1 + 0.1*noise());\r\n\t\t\r\n\t\tperiod_samples = period * SAMPLERATE;\r\n\t\tdur = period_samples * beetle_dur;\r\n\t\trdur = 1/dur;\r\n\t\t// 3000 .. 30,000 hz\r\n\t\tfreq = beetle_freq*beetle_frequency/period;\r\n\t\t// inc per sample\r\n\t\toinc = freq / SAMPLERATE;\r\n\t\t// 0.1 .. 1.0 (seconds)\r\n\t\tomod = 0.1 * beetle_mod;\r\n\t\tenvrpts = beetle_rpts + floor(abs(noise())*4);\r\n\t\t\r\n\t\tfilter = clamp(beetle_energy * beetle_filter, 0., 1.);\r\n\t\tfilter = 1.-(filter*filter);\r\n\t\t\r\n\t\tamp = beetle_scale;  \r\n\t\tamp0 = amp * (1.-beetle_pos_x)*(1.-beetle_pos_z);\r\n\t\tamp1 = amp * beetle_pos_x*beetle_pos_z;\r\n\t\t\r\n\t\tsmoothed = 0;\r\n\t\tophase = 0;\r\n\t\tfor (i=0; i<dur; i+=1) {\r\n\t\t\t// envelope:\r\n\t\t\tphase = i*rdur;\r\n\t\t\tshape = sample(env, phase*envrpts, boundmode=\"wrap\");\r\n\t\t\t// oscillator:\r\n\t\t\tophase += oinc * (1. + -0.001*omod*ophase); // + 0.001f*ophase);e*omod\r\n\t\t\to = sample(osc, ophase, boundmode=\"wrap\");\r\n\t\t\t// combine:\r\n\t\t\toe = o * shape*shape;\r\n\t\t\t// filter:\r\n\t\t\tsmoothed = mix(oe, smoothed, filter);\r\n\t\t\t// pan:\r\n\t\t\tpoke(ola, smoothed * amp0, olaindex+i, 0, 1, \r\n\t\t\t\toverdubmode=\"accum\", boundmode=\"wrap\");\r\n\t\t\tpoke(ola, smoothed * amp1, olaindex+i, 1, 1, \r\n\t\t\t\toverdubmode=\"accum\", boundmode=\"wrap\");\r\n\t\t}\r\n\t} \r\n\tpoke(sched, btime, b);\r\n}\r\n\r\nout1 = b;",
									"fontface" : 0,
									"fontname" : "<Monospaced>",
									"fontsize" : 12.0,
									"id" : "obj-9",
									"maxclass" : "codebox",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 13.0, 102.0, 836.0, 859.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-6",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 81.0, 27.0, 62.0, 22.0 ],
									"text" : "buffer env"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 13.0, 27.0, 61.0, 22.0 ],
									"text" : "buffer osc"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-28", 0 ],
									"order" : 0,
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 1 ],
									"midpoints" : [ 872.5, 89.0, 839.5, 89.0 ],
									"order" : 1,
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-1", 0 ],
									"source" : [ "obj-14", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 0 ],
									"source" : [ "obj-15", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-1", 2 ],
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-19", 0 ],
									"source" : [ "obj-28", 1 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 1 ],
									"midpoints" : [ 950.5, 152.5, 958.0, 152.5 ],
									"order" : 1,
									"source" : [ "obj-28", 2 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-30", 1 ],
									"midpoints" : [ 950.5, 152.5, 1035.0, 152.5 ],
									"order" : 0,
									"source" : [ "obj-28", 2 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-4", 0 ],
									"source" : [ "obj-28", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-29", 0 ],
									"order" : 1,
									"source" : [ "obj-32", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-30", 0 ],
									"order" : 0,
									"source" : [ "obj-32", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-7", 0 ],
									"source" : [ "obj-9", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 27.0, 251.988434000000012, 130.0, 22.0 ],
					"text" : "gen~ @title synthesize"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-13",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 471.0, 4.0, 34.0, 22.0 ],
					"text" : "init 1"
				}

			}
, 			{
				"box" : 				{
					"buffername" : "env",
					"id" : "obj-8",
					"labels" : 0,
					"maxclass" : "waveform~",
					"numinlets" : 5,
					"numoutlets" : 6,
					"outlettype" : [ "float", "float", "float", "float", "list", "" ],
					"patching_rect" : [ 471.0, 175.0, 256.0, 64.0 ],
					"ruler" : 0,
					"selectioncolor" : [ 0.929411764705882, 0.929411764705882, 0.352941176470588, 0.0 ]
				}

			}
, 			{
				"box" : 				{
					"buffername" : "osc",
					"id" : "obj-6",
					"labels" : 0,
					"maxclass" : "waveform~",
					"numinlets" : 5,
					"numoutlets" : 6,
					"outlettype" : [ "float", "float", "float", "float", "list", "" ],
					"patching_rect" : [ 471.0, 81.0, 256.0, 64.0 ],
					"ruler" : 0,
					"selectioncolor" : [ 0.929411764705882, 0.929411764705882, 0.352941176470588, 0.0 ]
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "float", "bang" ],
					"patching_rect" : [ 471.0, 151.0, 150.0, 22.0 ],
					"text" : "buffer~ env @samps 8192"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "float", "bang" ],
					"patching_rect" : [ 471.0, 57.0, 149.0, 22.0 ],
					"text" : "buffer~ osc @samps 8192"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "signal" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 8,
							"minor" : 0,
							"revision" : 5,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"classnamespace" : "dsp.gen",
						"rect" : [ 700.0, 496.0, 600.0, 450.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 148.0, 12.0, 74.0, 22.0 ],
									"text" : "buffer sched"
								}

							}
, 							{
								"box" : 								{
									"code" : "History init(1);\r\nif (init) {\t\r\n\tfor (i=0; i<dim(sched); i+=1) {\r\n\t\tpoke(sched, abs(noise()), i);\r\n\t}\r\n\tfor (i=0; i<dim(osc); i+=1) {\r\n\t\tphase = i/dim(osc);\r\n\t\tpoke(osc, sin(phase * TWOPI), i);\r\n\t}\r\n\tfor (i=0; i<dim(env); i+=1) {\r\n\t\tphase = i/dim(env);\r\n\t\t// skew env phase:\r\n\t\tphase = sin(PI * 0.5 * phase);\r\n\t\t// various env shapes:\r\n\t\t//v = 0.5 - cos(TWOPI * phase)*0.5;\r\n\t\t//v = 0.42 - 0.5*(cos(TWOPI*phase)) + 0.08*cos(4*PI*phase);\r\n\t\tv = 0.35875-0.48829*cos(2*PI*phase)+0.14128*cos(4*PI*phase)-0.01168*cos(6*PI*phase);\n\t\t//v = 0.22*(1. -1.93*cos(2*PI*phase)  +1.29*cos(4*PI*phase)  -0.388*cos(6*PI*phase)  +0.032*cos(8*PI*phase));\r\n\t\tpoke(env, v, i);\r\n\t}\r\n\t\r\n\tinit = 0;\r\n}",
									"fontface" : 0,
									"fontname" : "<Monospaced>",
									"fontsize" : 12.0,
									"id" : "obj-9",
									"maxclass" : "codebox",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 13.0, 46.0, 553.0, 355.0 ]
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-6",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 81.0, 12.0, 62.0, 22.0 ],
									"text" : "buffer env"
								}

							}
, 							{
								"box" : 								{
									"id" : "obj-5",
									"maxclass" : "newobj",
									"numinlets" : 0,
									"numoutlets" : 2,
									"outlettype" : [ "", "" ],
									"patching_rect" : [ 13.0, 12.0, 61.0, 22.0 ],
									"text" : "buffer osc"
								}

							}
 ],
						"lines" : [  ]
					}
,
					"patching_rect" : [ 471.0, 33.0, 134.0, 22.0 ],
					"text" : "gen~ @title maketables"
				}

			}
, 			{
				"box" : 				{
					"attr" : "beetle_base_period",
					"id" : "obj-18",
					"maxclass" : "attrui",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 266.0, 60.0, 150.0, 22.0 ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-10", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-1", 0 ],
					"source" : [ "obj-13", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-56", 0 ],
					"source" : [ "obj-17", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-12", 1 ],
					"source" : [ "obj-2", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-12", 0 ],
					"source" : [ "obj-2", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-20", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-21", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"source" : [ "obj-22", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-28", 0 ],
					"source" : [ "obj-27", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-2", 0 ],
					"order" : 1,
					"source" : [ "obj-28", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-25", 0 ],
					"order" : 0,
					"source" : [ "obj-28", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-54", 0 ],
					"source" : [ "obj-56", 0 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "audio.maxsnap",
				"bootpath" : "~/Documents/Max 8/Snapshots",
				"patcherrelativepath" : "../../Documents/Max 8/Snapshots",
				"type" : "mx@s",
				"implicit" : 1
			}
, 			{
				"name" : "bin2buf~.mxo",
				"type" : "iLaX"
			}
 ],
		"autosave" : 0,
		"snapshot" : 		{
			"filetype" : "C74Snapshot",
			"version" : 2,
			"minorversion" : 0,
			"name" : "snapshotlist",
			"origin" : "jpatcher",
			"type" : "list",
			"subtype" : "Undefined",
			"embed" : 1,
			"snapshotlist" : 			{
				"current_snapshot" : -1,
				"entries" : [ 					{
						"filetype" : "C74Snapshot",
						"version" : 2,
						"minorversion" : 0,
						"name" : "audio",
						"origin" : "audio",
						"type" : "patcher",
						"subtype" : "Undefined",
						"embed" : 0,
						"fileref" : 						{
							"name" : "audio",
							"filename" : "audio.maxsnap",
							"filepath" : "~/Documents/Max 8/Snapshots",
							"filepos" : -1,
							"snapshotfileid" : "612bf4929fdaa8764ad7d99a690ba9de"
						}

					}
 ]
			}

		}

	}

}
