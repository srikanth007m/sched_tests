/*
 * Copyright (C) 2015 ARM Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* string - blocks of text used in load generator */

const char help_text[]=
		"Load sequence language:\n"
		"A load sequence is made up of a number of timed points, where the time is specified in milliseconds.\n"
		"All times are relative to the start of the load trace, NOT the previous timed point.\n"
		"At each point, the load to be generated relative to a given CPU's calibration is given as a percentage.\n"
		"For example, to start a 50% load equivalent to that of CPU0 at 1s, the timed point looks like:\n"
		"    1000,0.50\n"
		"Which follows the standard format of:\n"
		"<time in ms>,<cpu number>.<percentage load>\n"
		"The timed points can be joined together to form a sequence using a separator.\n"
		"    1000,0.50:2000,1.100\n"
		"This starts a load equivalent to 50% of CPU0 after 1s, and after 2s switches to 100% of CPU1\n\n"
		"Interpolated Loads:\n"
		"A timed point can be extended to generate a linear interpolation at 10ms intervals between two calibration points.\n"
		"To do this, use a - symbol between two timed points, like so:\n"
		"    1000,0.50-2000,1.100\n"
		"This represents a linear interpolation over 1s between 50% of CPU0 and 100% of CPU1.\n\n"
		"CPU Identifiers:\n"
		"When selecting load to generate, the assumption is that task placement will be controlled by the environment.\n"
		"There is no control in the language over where the load is executed - it runs in the context of the main thread "
		"of the running executable.\n"
		"CPU identifiers used in the language refer to the calibration data for a given CPU.\n"
		"There are however two special CPU identifiers which we have not so far shown.\n"
		"B or b - selects the CPU with the largest calibrated number for 100% load\n"
		"L or l - selects the CPU with the lowest calibrated number for 100% load\n"
		"e.g. generate load equivalent to 50% of the biggest CPU available looks like:\n"
		"    1000,B.50\n\n"
		"Ending a load sequence:\n"
		"A load sequence does not need to have an explicit end. When one is not present the previous "
		"load point will be maintained until the exe is killed.\n"
		"If you wish to end the load generation at a specific time, then you can use end in place of a CPU:\n"
		"    0,0.0:1000,0.100:2000,end\n"
		"is a load sequence starting at 100% after 1s of idle and ending after 2s.\n"
		"The \'end tag\' must be the last tag and there can be only one.\n"
		"\n"
		"Load Sequence Files:\n"
		"In order to make it easy to share test loads, the load sequences can be stored in a plain ascii text "
		"file. The format is exactly the same and line endings are ignored.\n"
		"For convenience, if a separator is not present but a line ending is, the default : separator is assumed.\n"
		"\n"
		"Changing Priority:\n"
		"You can optionally set the process priority in any step by including an additional dot field in a step. "
		"The priority is set like \'nice\', so is between -20 and 20 with the default being read from the system.\n"
		"e.g. this load sequence starts at \'system default\' nice, goes to nice -20 at 1s, stays at nice -20 "
		"at 2s and to nice 0 at 3s:\n"
		"    0,l.10:1000,l.10.-20:2000,l.10:3000,l.10.0\n"
		"NOTE:\n"
		"1. If a nice value is not set at the beginning, the current priority is read and used.\n"
		"2. When a nice value is set, it remains in place until you change it.\n"
		"";

const char help_desc[]=
	"Display detailed help about load sequence formats";

const char calibrate_desc[]=
	"Run calibration loop on all available CPUs and save the resulting calibration data.\n"
	"Calibration should be performed as ROOT so that CPUFreq governors can be set to performance during the test";

const char calibfile_desc[]=
	"Change location and filename of calibration data file, relative to current directory.\n";

const char display_desc[]=
	"Load and display the calibration tables";

const char loadseq_desc[]=
	"Generate and execute a load profile using a string";

const char loadseqfile_desc[]=
	"Generate and execute a load profile using a load sequence file";
