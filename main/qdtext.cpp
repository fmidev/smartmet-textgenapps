// ======================================================================
/*!
 * \file
 * \brief generate farmer forecasts into counties etc
 */
// ======================================================================

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <boost/tokenizer.hpp>
#include <calculator/Config.h>
#include <calculator/Settings.h>
#include <calculator/WeatherArea.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiFileSystem.h>
#include <newbase/NFmiSettings.h>
#include <newbase/NFmiStringTools.h>
#include <textgen/Dictionary.h>
#include <textgen/DictionaryFactory.h>
#include <textgen/Document.h>
#include <textgen/MessageLogger.h>
#include <textgen/PostGISDataSource.h>
#include <textgen/TextFormatter.h>
#include <textgen/TextFormatterFactory.h>
#include <textgen/TextGenerator.h>
#include <cstdlib>  // putenv
#include <ctime>    // tzset
#include <fstream>
#include <list>
#include <map>
#include <ogrsf_frmts.h>
#include <stdexcept>

using namespace std;
using namespace boost;

using TextList = std::list<std::string>;
using TextMap = std::map<std::string, TextList>;

static TextMap textMap;

// ----------------------------------------------------------------------
/*!
 * \brief Write farmer forecast to a file or files
 *
 * \param theOutDir The output directory
 * \param theFilenames The files into which to write
 * \param theText The text to write
 */
// ----------------------------------------------------------------------

void write_forecasts(const string& theOutDir, const string& theFilenames, const string& theText)
{
  MessageLogger log("write_forecasts");
  const vector<string> filenames = NFmiStringTools::Split(theFilenames);

  for (vector<string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it)
  {
    const string filename = theOutDir + '/' + *it;

    log << "writing " << filename << endl;

    ofstream out(filename.c_str());
    if (!out) throw runtime_error("Could not open " + filename + " for writing");

    string encoding = Settings::optional_string("qdtext::encoding", "Latin1");
    if (encoding != "UTF-8")
      out << boost::locale::conv::from_utf(theText, encoding);
    else
      out << theText;

    out.close();
    if (out.bad()) throw runtime_error("Error while writing " + filename);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Save Document in all requested languages
 *
 * \param theDocument The document to save
 * \param theDictionary The dictionary to use
 * \param theArea The area name
 */
// ----------------------------------------------------------------------

void save_forecasts(const TextGen::Document& theDocument,
                    boost::shared_ptr<TextGen::Dictionary>& theDictionary,
                    const string& theArea)
{
  MessageLogger log("save_forecasts");

  const string productnames = Settings::require_string("qdtext::products");
  const vector<string> products = NFmiStringTools::Split(productnames);

  for (vector<string>::const_iterator it = products.begin(); it != products.end(); ++it)
  {
    const string product = "qdtext::product::" + *it;
    const string var = product + "::filename::" + theArea;

    // if no filename is given, print to stdout (indicated by '-')
    const string filenames = Settings::optional_string(var, "-");
    const string lang = Settings::require_string(product + "::language");
    const string form = Settings::require_string(product + "::formatter");

    if (lang != theDictionary->language()) theDictionary->init(lang);

    boost::shared_ptr<TextGen::TextFormatter> formatter(
        TextGen::TextFormatterFactory::create(form));
    formatter->dictionary(theDictionary);
    formatter->setProductName(*it);

    std::string forecast = formatter->format(theDocument);

    if (form == "html") forecast += Settings::optional_string("html__append", "");

    // save texts first to map
    if (textMap.find(filenames) == textMap.end())
    {
      TextList textList;
      textMap.insert(make_pair(filenames, textList));
    }

    TextList& textList = textMap[filenames];
    textList.push_back(forecast);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a WeatherArea from the descriptions
 *
 * The descriptions are in variable textgen::areas::areaname
 * subvariables area, polygon, location, longitude, latitude and radius.
 * The allowed combinations are
 *
 *  -# polygon + radius
 *  -# polygon
 *  -# location + radius
 *  -# longitude + latitude + radius
 *  -# longitude + latitude
 *
 * If textgen::areas::areaname itself is set, it is passed to
 * to the WeatherArea constructor as is. Otherwise the subvariables
 * are combined to create the constructor argument. If no variables
 * are set at all, the name itself is passed on to the WeatherArea
 * constructor.
 *
 * The area is named after textgen::areas::areaname::name if it is set,
 * otherwise areaname is used.
 */
// ----------------------------------------------------------------------

const TextGen::WeatherArea make_area(const string& theName)
{
  const string var = "qdtext::areas::" + theName;

  const string namevar = theName + "::name";
  const string polygonvar = theName + "::polygon";
  const string locationvar = theName + "::location";
  const string longitudevar = theName + "::longitude";
  const string latitudevar = theName + "::latitude";
  const string radiusvar = theName + "::radius";

  ostringstream spec;

  if (Settings::isset(var))
    spec << Settings::require_string(var);
  else
  {
    if (Settings::isset(polygonvar))
      spec << Settings::require_string(polygonvar);
    else if (Settings::isset(locationvar))
      spec << Settings::require_string(locationvar);
    else if (Settings::isset(longitudevar) && Settings::isset(latitudevar))
      spec << Settings::require_double(longitudevar) << ','
           << Settings::require_double(latitudevar);
    else if (Settings::isset(longitudevar) || Settings::isset(latitudevar))
      throw runtime_error(
          "Area '" + theName +
          "' does not have a valid description in variable qdtext::areas::" + theName);
    else
      spec << theName;

    if (Settings::isset(radiusvar)) spec << ':' << Settings::require_double(radiusvar);
  }

  string name = Settings::optional_string(namevar, theName);

  return TextGen::WeatherArea(spec.str(), name);
}

// ----------------------------------------------------------------------
/*!
 * \brief Make the farmer forecasts
 */
// ----------------------------------------------------------------------

void make_forecasts()
{
  MessageLogger log("make_forecasts");

  using namespace NFmiStringTools;
  using namespace TextGen;
  log << "qdtext version " << TextGenerator::version() << endl;
  log << "The configuration file:" << endl << NFmiSettings::ToString() << endl << endl;

  // Check required variables

  const vector<string> areas = Split(Settings::require_string("qdtext::areas"));
  const string dictionaryname = Settings::require_string("qdtext::dictionary");
  const string default_timezone = Settings::require_string("qdtext::timezone");

  boost::shared_ptr<Dictionary> dict(DictionaryFactory::create(dictionaryname));

  TextGenerator generator;
  if (Settings::isset("qdtext::forecasttime"))
    generator.time(Settings::require_time("qdtext::forecasttime"));

  BrainStorm::PostGISDataSource pgis;
  bool usePostGISData(Settings::isset("qdtext::areas::postgis::database"));

  if (usePostGISData)
  {
    const std::string host(Settings::require_string("qdtext::areas::postgis::host"));
    const std::string port(Settings::require_string("qdtext::areas::postgis::port"));
    const std::string dbname(Settings::require_string("qdtext::areas::postgis::database"));
    const std::string user(Settings::require_string("qdtext::areas::postgis::username"));
    const std::string password(Settings::require_string("qdtext::areas::postgis::password"));
    const std::string schema(Settings::require_string("qdtext::areas::postgis::schema"));
    const std::string table(Settings::require_string("qdtext::areas::postgis::table"));
    const std::string field(Settings::require_string("qdtext::areas::postgis::field"));
    const std::string encoding(
        Settings::optional_string("qdtext::areas::postgis::encoding", "latin1"));
    std::string log_message;

    pgis.readData(host, port, dbname, user, password, schema, table, field, encoding, log_message);

    if (!log_message.empty()) log << log_message << endl;
  }

  for (vector<string>::const_iterator it = areas.begin(); it != areas.end(); ++it)
  {
    log << "Area " + *it << endl;
    Settings::set("html__append", "");

    // timezone can be defined for area
    string variable = "qdtext::timezone::" + *it;
    string timezone = Settings::optional_string(variable, default_timezone);
    TextGenPosixTime::SetThreadTimeZone(timezone);

    log << *it << " timezone is " << timezone << endl;

    if (usePostGISData)
    {
      std::string area_name(Settings::require_string("qdtext::areas::" + *it));
      if (pgis.geoObjectExists(area_name))
      {
        if (pgis.isPolygon(area_name))
        {
          stringstream svg_string_stream(pgis.getSVGPath(area_name));
          NFmiSvgPath svgPath;
          svgPath.Read(svg_string_stream);
          const TextGen::WeatherArea area(svgPath, *it);
          const TextGen::Document document = generator.generate(area);
          save_forecasts(document, dict, *it);
        }
        else  // if not polygon, it must be a point
        {
          std::pair<double, double> std_point(pgis.getPoint(area_name));
          NFmiPoint point(std_point.first, std_point.second);
          const TextGen::WeatherArea area(point, *it);
          const TextGen::Document document = generator.generate(area);
          save_forecasts(document, dict, *it);
        }
      }
      else
      {
        log << "NOTE!! " + *it << " (" << area_name << ") not found in PostGIS database!" << endl;
      }
    }
    else
    {
      const TextGen::WeatherArea area = make_area(*it);
      const TextGen::Document document = generator.generate(area);
      save_forecasts(document, dict, *it);
    }
  }

  const string outdir = Settings::require_string("qdtext::outdir");

  // iterate the map and write out the forecasts
  for (TextMap::iterator tm_iter = textMap.begin(); tm_iter != textMap.end(); ++tm_iter)
  {
    std::ostringstream string_stream;
    for (TextList::const_iterator tl_iter = tm_iter->second.begin();
         tl_iter != tm_iter->second.end();
         ++tl_iter)
    {
      std::string str(*tl_iter);
      string_stream << str;
    }
    std::string outfile(tm_iter->first);

    // if output file is '-' print to stdout
    if (outfile.compare("-") == 0)
    {
      string encoding = Settings::optional_string("qdtext::encoding", "Latin1");
      if (encoding != "UTF-8")
        cout << boost::locale::conv::from_utf(string_stream.str(), encoding);
      else
        cout << string_stream.str();
    }
    else
      write_forecasts(outdir, outfile, string_stream.str());
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Read the command line arguments
 *
 * Option -c [conffile] sets the configuration file.
 * Option -d [path] sets the output directory
 * Option -a [areas] sets the areas to be processed
 * Option -l [language] sets the output languages
 */
// ----------------------------------------------------------------------

bool read_cmdline(int argc, const char* argv[])
{
  // Now read command line options

  NFmiCmdLine cmdline(argc, argv, "hc!d!a!");

  if (cmdline.Status().IsError()) throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h'))
  {
    cout << "Usage: qdtext configfile | -h" << endl
         << endl
         << "\tconfigfile: fully specified name of the configuration file" << endl
         << endl
         << "Options: " << endl
         << endl
         << "\t-h\tprints this usage information" << endl;
    return false;
  }

  if (cmdline.NumberofParameters() != 1)
    throw runtime_error("Exactly one parameter is expected on the command line");

  // The settings must be read before the options so that we can
  // override the settings in the file

  const string filename = cmdline.Parameter(1);
  if (filename.empty()) throw runtime_error("Empty argument for option -c");
  if (!NFmiFileSystem::FileExists(filename))
    throw runtime_error("File " + filename + " does not exist");
  NFmiSettings::Read(filename);

  Settings::set(NFmiSettings::ToString());

  if (cmdline.isOption('d'))
  {
    const string outdir = cmdline.OptionValue('d');
    if (outdir.empty()) throw runtime_error("Empty argument for option -d");
    if (!NFmiFileSystem::DirectoryExists(outdir))
      throw runtime_error("-d argument " + outdir + " does not exist");
    Settings::set("qdtext::outputdir", outdir);
  }

  if (cmdline.isOption('a'))
  {
    const string areas = cmdline.OptionValue('a');
    if (areas.empty()) throw runtime_error("Empty argument for option -a");
    Settings::set("qdtext::areas", areas);
  }

  if (cmdline.isOption('l'))
  {
    const string languages = cmdline.OptionValue('l');
    if (languages.empty()) throw runtime_error("Empty argument for option -l");
    Settings::set("qdtext::languages", languages);
  }

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main work subroutine for the main program
 *
 * The main program traps all exceptions throw in here.
 */
// ----------------------------------------------------------------------

int run(int argc, const char* argv[])
{
  // Read the global settings first

  NFmiSettings::Init();

  Fmi::Config::set("qdtext::outputdir", ".");

  // Read the command line arguments

  if (!read_cmdline(argc, argv)) return 0;

  // Settings are now fine

  const string logfile = Settings::optional_string("qdtext::logfile", "");
  if (!logfile.empty()) MessageLogger::open(logfile);
  MessageLogger::indent(' ');
  MessageLogger::indentstep(2);
  MessageLogger::timestamp(true);

  // Establish locale too
  const string loc = Settings::optional_string("qdtext::locale", "fi_FI.UTF-8");
  boost::locale::generator gen;
  std::locale::global(gen(loc));
  // Make the forecasts

  make_forecasts();

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 *
 * The main program is only an error trapping driver for domain
 */
// ----------------------------------------------------------------------

int main(int argc, const char* argv[]) try
{
  return run(argc, argv);
}
catch (const std::exception& e)
{
  cerr << "Error: Caught an exception:" << endl << "--> " << e.what() << endl;
  return 1;
}
catch (...)
{
  cerr << "Error: Caught an unknown exception" << endl;
  return 1;
}

// ======================================================================
