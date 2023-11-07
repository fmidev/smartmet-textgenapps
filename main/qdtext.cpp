// ======================================================================
/*!
 * \file
 * \brief generate farmer forecasts into counties etc
 */
// ======================================================================

#include <boost/algorithm/string.hpp>
#include <macgyver/DateTime.h>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <boost/tokenizer.hpp>
#include <calculator/Config.h>
#include <calculator/Settings.h>
#include <calculator/WeatherArea.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
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

using std::cerr;
using std::cout;
using std::endl;
using std::ofstream;
using std::ostringstream;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::vector;

using TextList = std::list<std::string>;
using TextMap = std::map<std::string, TextList>;

namespace
{
TextMap textMap;
}

// ----------------------------------------------------------------------
/*!
 * \brief Write farmer forecast to a file or files
 *
 * \param theOutDir The output directory
 * \param theFilenames The files into which to write
 * \param theText The text to write
 */
// ----------------------------------------------------------------------

void write_forecasts(const string& theOutDir,
                     const string& theFilenames,
                     const string& theText,
                     const Fmi::DateTime& theTime)
{
  MessageLogger log("write_forecasts");
  const vector<string> filenames = NFmiStringTools::Split(theFilenames);

  auto timeinfo = to_tm(theTime);

  for (const auto& file : filenames)
  {
    string filename = std::string(theOutDir).append("/").append(file);
    filename = fmt::format(fmt::runtime(filename), timeinfo);

    log << "writing " << filename << endl;

    ofstream out(filename.c_str());
    if (!out)
      throw runtime_error("Could not open " + filename + " for writing");

    string encoding = Settings::optional_string("qdtext::encoding", "Latin1");
    if (encoding != "UTF-8")
      out << boost::locale::conv::from_utf(theText, encoding);
    else
      out << theText;

    out.close();
    if (out.bad())
      throw runtime_error("Error while writing " + filename);
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

  for (const auto& prodname : products)
  {
    const string product = "qdtext::product::" + prodname;
    const string var = std::string(product).append("::filename::").append(theArea);

    // if no filename is given, print to stdout (indicated by '-')
    const string filenames = Settings::optional_string(var, "-");
    const string lang = Settings::require_string(product + "::language");
    const string form = Settings::require_string(product + "::formatter");

    if (lang != theDictionary->language())
      theDictionary->changeLanguage(lang);

    boost::shared_ptr<TextGen::TextFormatter> formatter(
        TextGen::TextFormatterFactory::create(form));
    formatter->dictionary(theDictionary);
    formatter->setProductName(prodname);

    std::string forecast = formatter->format(theDocument);

    if (form == "html")
      forecast += Settings::optional_string("html__append", "");

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

TextGen::WeatherArea make_area(const string& theName)
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

    if (Settings::isset(radiusvar))
      spec << ':' << Settings::require_double(radiusvar);
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

  log << "qdtext version " << TextGen::TextGenerator::version() << endl;
  log << "The configuration file:" << endl << NFmiSettings::ToString() << endl << endl;

  // Check required variables

  const vector<string> areas = NFmiStringTools::Split(Settings::require_string("qdtext::areas"));
  const string dictionaryname = Settings::require_string("qdtext::dictionary");
  const string default_timezone = Settings::require_string("qdtext::timezone");
  const string s_languages =
      Settings::optional_string("qdtext::supported_languages", "fi,sv,en,sonera");
  std::vector<std::string> supported_languages;
  boost::algorithm::split(supported_languages, s_languages, boost::algorithm::is_any_of(","));
  boost::shared_ptr<TextGen::Dictionary> dict(TextGen::DictionaryFactory::create(dictionaryname));
  const std::string dictionaryId = dict->getDictionaryId();

  if (dictionaryId == "mysql" || dictionaryId == "postgresql")
  {
    std::vector<std::string> missing_params;
    if (!Settings::isset("textgen::host"))
      missing_params.emplace_back("textgen::host");
    if (!Settings::isset("textgen::user"))
      missing_params.emplace_back("textgen::user");
    if (!Settings::isset("textgen::passwd"))
      missing_params.emplace_back("textgen::passwd");
    if (!Settings::isset("textgen::database"))
      missing_params.emplace_back("textgen::database");

    if (dictionaryId == "postgresql")
    {
      if (!Settings::isset("textgen::port"))
        Settings::set("textgen::port", "5432");
      if (!Settings::isset("textgen::encoding"))
        Settings::set("textgen::encoding", "UTF8");
      if (!Settings::isset("textgen::connect_timeout"))
        Settings::set("textgen::connect_timeout", "30");
    }

    if (missing_params.size() > 0)
    {
      std::string msg = ("Settings missing for " + dictionaryId + ": ");
      for (const auto& mp : missing_params)
        msg += (mp + ",");
      msg.resize(msg.size() - 1);
      throw runtime_error(msg);
    }
  }

  for (const auto& lang : supported_languages)
    dict->init(lang);

  TextGen::TextGenerator generator;
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

    if (!log_message.empty())
      log << log_message << endl;
  }

  auto now = Fmi::SecondClock::universal_time();

  for (const auto& areaname : areas)
  {
    log << "Area " + areaname << endl;
    Settings::set("html__append", "");

    // timezone can be defined for area
    string variable = "qdtext::timezone::" + areaname;
    string timezone = Settings::optional_string(variable, default_timezone);
    TextGenPosixTime::SetThreadTimeZone(timezone);

    log << areaname << " timezone is " << timezone << endl;

    if (usePostGISData)
    {
      std::string area_name(Settings::require_string("qdtext::areas::" + areaname));
      if (pgis.geoObjectExists(area_name))
      {
        if (pgis.isPolygon(area_name))
        {
          stringstream svg_string_stream(pgis.getSVGPath(area_name));
          NFmiSvgPath svgPath;
          svgPath.Read(svg_string_stream);
          const TextGen::WeatherArea area(svgPath, areaname);
          const TextGen::Document document = generator.generate(area);
          save_forecasts(document, dict, areaname);
        }
        else  // if not polygon, it must be a point
        {
          std::pair<double, double> std_point(pgis.getPoint(area_name));
          NFmiPoint point(std_point.first, std_point.second);
          const TextGen::WeatherArea area(point, areaname);
          const TextGen::Document document = generator.generate(area);
          save_forecasts(document, dict, areaname);
        }
      }
      else
      {
        log << "NOTE!! " + areaname << " (" << area_name << ") not found in PostGIS database!"
            << endl;
      }
    }
    else
    {
      const TextGen::WeatherArea area = make_area(areaname);
      const TextGen::Document document = generator.generate(area);
      save_forecasts(document, dict, areaname);
    }
  }

  const string outdir = Settings::require_string("qdtext::outdir");

  // iterate the map and write out the forecasts
  for (const auto& textmap : textMap)
  {
    std::ostringstream string_stream;
    for (const auto& tl : textmap.second)
      string_stream << tl;

    std::string outfile(textmap.first);

    // if output file is '-' print to stdout
    if (outfile == "-")
    {
      string encoding = Settings::optional_string("qdtext::encoding", "Latin1");
      if (encoding != "UTF-8")
        cout << boost::locale::conv::from_utf(string_stream.str(), encoding);
      else
        cout << string_stream.str();
    }
    else
      write_forecasts(outdir, outfile, string_stream.str(), now);
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

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h') != 0)
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
  if (filename.empty())
    throw runtime_error("Empty argument for option -c");
  if (!NFmiFileSystem::FileExists(filename))
    throw runtime_error("File " + filename + " does not exist");

  NFmiSettings::Read(filename);

  Settings::set(NFmiSettings::ToString());

  if (cmdline.isOption('d') != 0)
  {
    const string outdir = cmdline.OptionValue('d');
    if (outdir.empty())
      throw runtime_error("Empty argument for option -d");
    if (!NFmiFileSystem::DirectoryExists(outdir))
      throw runtime_error("-d argument " + outdir + " does not exist");
    Settings::set("qdtext::outputdir", outdir);
  }

  if (cmdline.isOption('a') != 0)
  {
    const string areas = cmdline.OptionValue('a');
    if (areas.empty())
      throw runtime_error("Empty argument for option -a");
    Settings::set("qdtext::areas", areas);
  }

  if (cmdline.isOption('l') != 0)
  {
    const string languages = cmdline.OptionValue('l');
    if (languages.empty())
      throw runtime_error("Empty argument for option -l");
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

  if (!read_cmdline(argc, argv))
    return 0;

  // Settings are now fine

  const string logfile = Settings::optional_string("qdtext::logfile", "");
  if (!logfile.empty())
    MessageLogger::open(logfile);
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

int main(int argc, const char* argv[])
try
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
