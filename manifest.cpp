// Opal Manifest
// for Unix-like systems
// version 1
// written in 2024 by DonaldET3

#include <cstdlib>
// EXIT_FAILURE
// EXIT_SUCCESS

#include <iostream>
using std::cerr;
using std::cout;
using std::cin;

#include <exception>
using std::exception;

#include <stdexcept>
using std::runtime_error;

#include <ios>
// failbit, badbit
using std::ios_base;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <filesystem>
namespace fs = std::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

// program options
struct opt_struct {
	bool verbose {false};

	// file types
	bool regular {false}, directory {false},
		chr_dev {false}, blk_dev {false},
		symlink {false}, fifo {false};

	// update options
	bool update {false}, add {false}, remove {false}, modified {false};

	// metadata types
	bool size {false}, mtime {false};

	// symlink options
	bool cmd_lnk {false}, // follow symlinks specified in the command line
		all_lnk {false}; // follow all symlinks
};

fs::file_status get_stat(bool follow_link, fs::directory_entry file)
{
	fs::file_status s;

	if(follow_link) s = file.status();
	else s = file.symlink_status();

	return s;
}

void update_manifest(vector<string>& fnames, struct opt_struct& opts)
{
}

void make_manifest(vector<string>& fnames, struct opt_struct& opts)
{
	cout << "OUmanifest 1\n\n";

	if(fnames.size())
	{
		for(string& fn : fnames)
		{
			try
			{
				// get file metadata
				fs::directory_entry file(fs::path(fn, fs::path::format::native_format));
				fs::file_status s = get_stat(opts.cmd_lnk, file);

				if(!fs::exists(s))
				{
					cerr << '"' << fn << "\" does not exist\n";
					continue;
				}

				// process directory
				if(fs::is_directory(s)) proc_dir(file, opts);
				// or just write file record
				else w_file_r(file, opts);
			}
			catch(fs::filesystem_error& e)
			{cerr << e.what() << '\n';}
		}
	}
	else
	{
		// process pwd
		fs::directory_entry file(fs::path(".", fs::path::format::generic_format));
		proc_dir(file, opts);
	}
}

void type_opts(struct opt_struct& opts, vector<string>& types)
{
	for(string& s : types)
		for(char c : s)
			switch(c)
			{
				case 'r': opts.regular = true; break;
				case 'd': opts.directory = true; break;
				case 'c': opts.chr_dev = true; break;
				case 'b': opts.blk_dev = true; break;
				case 'l': opts.symlink = true; break;
				case 'f': opts.fifo = true; break;
				default: throw runtime_error(string("\"") + c +
					"\" does not correspond to a file type");
			}
}

void update_opts(struct opt_struct& opts, vector<string>& update)
{
	opts.update = true;

	for(string& s : update)
		for(char c : s)
			switch(c)
			{
				case 'a': opts.add = true; break;
				case 'r': opts.remove = true; break;
				case 'm': opts.modified = true; break;
				default: throw runtime_error(string("\"") + c +
					"\" is not an update type");
			}
}

void metadata_opts(struct opt_struct& opts, vector<string>& metadata)
{
	for(string& s : metadata)
		for(char c : s)
			switch(c)
			{
				case 's': opts.size = true; break;
				case 'm': opts.mtime = true; break;
				default: throw runtime_error(string("\"") + c +
					"\" does not correspond to a metadata type");
			}
}

int main(int argc, char **argv)
{
	try
	{
		vector<string> types, update, metadata, fnames;
		struct opt_struct opts {};

		// help message
		string message = "Opal Manifest\nversion 1\n\n"
			"This program creates a manifest file containing metadata of files in a file hierarchy. The files are specified on the command line and the manifest file is output to standard output. A manifest file that needs to be updated can be input through standard input. The t, u, and m options are followed by characters that specify their behavior.\n\noptions\n";

		po::options_description desc(message);
		desc.add_options() // visible command line options
			("help,h", "output help and exit")
			("verbose,v", po::bool_switch(&opts.verbose),
				"verbose mode")
			("types,t", po::value<vector<string>>(&types),
				"file types to output\n"
				"r: regular files\n"
				"d: directories\n"
				"c: character devices\n"
				"b: block devices\n"
				"l: symbolic links\n"
				"f: fifos (pipes)")
			("update,u", po::value<vector<string>>(&update),
				"update mode\n"
				"a: add\n"
				"r: remove\n"
				"m: update modified")
			("metadata,m", po::value<vector<string>>(&metadata),
				"types of metadata to include\n"
				"s: file size\n"
				"m: modification time")
			("command-links,H", po::bool_switch(&opts.cmd_lnk),
				"process the files pointed at by symlinks specified in the command line instead of the symlinks themselves")
			("all-links,L", po::bool_switch(&opts.all_lnk),
				"process the files pointed at by all symlinks encountered in the hierarchy instead of the symlinks themselves")
		;

		// add positional arguments
		po::options_description cmdline_options;
		cmdline_options.add(desc).add_options()
			("file-name", po::value<vector<string>>(&fnames), "");

		po::positional_options_description p;
		p.add("file-name", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(cmdline_options).positional(p).run(), vm);
		po::notify(vm); // put values in the variables

		if(vm.count("help"))
		{
			cerr << desc << '\n';
			return EXIT_SUCCESS;
		}

		if(vm.count("types")) type_opts(opts, types);
		if(vm.count("update")) update_opts(opts, update);
		if(vm.count("metadata")) metadata_opts(opts, metadata);
		if(opts.all_lnk) opts.cmd_lnk = true;

		// enable stream exceptions
		cin.exceptions(ios_base::badbit);
		cout.exceptions(ios_base::failbit | ios_base::badbit);

		if(opts.update) update_manifest(fnames, opts);
		else make_manifest(fnames, opts);
	}
	catch(exception& e)
	{
		cerr <<  e.what() << '\n';
		return EXIT_FAILURE;
	}
	catch(...)
	{
		cerr << "exception of unknown type\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

