/*!
	\file TestParser.cpp
	\author Gregory Diamos <gregory.diamos@gatech.edu>
	\date Monday January 19, 2009
	\brief The source file for the TestParser class
*/


#ifndef TEST_PARSER_CPP_INCLUDED
#define TEST_PARSER_CPP_INCLUDED


#include "boost/filesystem.hpp"
#include <queue>
#include <fstream>

#include <ocelot/parser/interface/PTXParser.h>
#include <ocelot/parser/test/TestParser.h>

#include <hydrazine/implementation/ArgumentParser.h>
#include <hydrazine/implementation/macros.h>
#include <hydrazine/implementation/debug.h>

#ifdef REPORT_BASE
#undef REPORT_BASE
#endif

#define REPORT_BASE 0

namespace fs = boost::filesystem;

namespace test
{

	TestParser::StringVector TestParser::_getFileNames() const
	{
	
		StringVector names;
		
		fs::path path = input;
		
		if( fs::is_directory( path ) )
		{
			std::queue< fs::directory_iterator > directories;
			directories.push( path );
			
			fs::directory_iterator end;
			
			while( !directories.empty() )
			{
				for( fs::directory_iterator 
					file = directories.front(); 
					file != end; ++file )
				{
					if( fs::is_directory( file->status() ) && recursive )
					{
						directories.push( file->path() );
					}
					else if( fs::is_regular_file( file->status() ) )
					{
						if( file->path().extension() == ".ptx" )
						{
							names.push_back( file->path().string() );
						}
					}
				}
				directories.pop();
			}
		}
		else if( fs::is_regular_file( path ) )
		{
			if( path.extension() == ".ptx" )
			{
				names.push_back( path.string() );
			}
		}
		
		return names;
	}


	bool TestParser::_testParse()
	{
	
		report( " Parsing file " << ptxFile );
		
		std::stringstream stream;
		std::ifstream file( ptxFile.c_str() );
		
		parser::PTXParser parser;
		
		parser.fileName = ptxFile;
		ir::Module first = parser.parse( file );
		
		report( " Dumping to stream" );
		
		first.write( stream );

		std::string outputFile = ptxFile + ".parsed";
		
		if( output )
		{
		
			std::ofstream outFile( outputFile.c_str() );
			first.write( outFile );
			outFile.close();
		
		}
		
		report( " Running second parse pass" );
		parser.fileName = outputFile;
		ir::Module second = parser.parse( stream );
		
		if( first.statements.size() != second.statements.size() )
		{
		
			status << "First pass parsed " << first.statements.size()
				<< " statements while second parsed " 
				<< second.statements.size() << "\n";
			return false;
		
		}
		
		for( ir::Module::StatementVector::iterator 
			fi = first.statements.begin(), 
			si = second.statements.begin(); fi != first.statements.end() && 
			si != second.statements.end(); ++fi, ++si )
		{
		
			if( !( si->toString() == fi->toString() ) )
			{
			
				unsigned int index = fi - first.statements.begin();
				status << "At index " << index << " first pass parsed \"" 
					<< fi->toString() << "\" while second parsed \"" 
					<< si->toString() << "\"\n";
				return false;
			
			}
		
		}
		
		return true;
	
	}

	bool TestParser::doTest()
	{

		StringVector files = _getFileNames();
		
		report( "Parsing the following files:\n " 
			<< hydrazine::toString( files.begin(), files.end(), "\n " )  );
		
		for( StringVector::iterator fi = files.begin(); 
			fi != files.end(); ++fi )
		{
		
			ptxFile = *fi;
		
			if(  !_testParse( ) )
			{
		
				status << "For file " << ptxFile 
					<< ", Test Point 1 (Parse): Failed\n";
				return false;
		
			}
		
			status << "For file " << ptxFile 
				<< ", Test Point 1 (Parse): Passed\n";
			
		}
			
		return true;
	
	}

	TestParser::TestParser()
	{
	
		name = "TestParser";
		
		description = "A test for the PTXParser class. Test Points: 1) Load a";
		description += " PTX file and run it through the parser generating a";
		description += " module.  Write the module to an intermediate stream.";
		description += "  Parse the stream again generating a new module, ";
		description += "compare both to make sure that they match.";
	
	}

}

int main( int argc, char** argv )
{

	hydrazine::ArgumentParser parser( argc, argv );
	test::TestParser test;
	parser.description( test.testDescription() );

	parser.parse( "-i", test.input, "../tests/ptx",
		"Input directory to search for ptx files." );
	parser.parse( "-r", test.recursive, true, 
		"Recursively search directories.");
	parser.parse( "-o", test.output, false,
		"Print out the internal representation of each parsed file." );
	parser.parse( "-s", test.seed, 0,
		"Set the random seed, 0 implies seed with time." );
	parser.parse( "-v", test.verbose, false, "Print out info after the test." );
	parser.parse();
	
	test.test();

	return test.passed();

}

#endif
