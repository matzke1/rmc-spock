static const char *purpose = "download all known source code";
static const char *description =
    "The purpose of this program is to initialize the download cache by attempting to download all known versions of all "
    "software packages. This is useful on systems that are behind a firewall that normally blocks outgoing connections.";

#include <Spock/Context.h>
#include <Spock/DefinedPackage.h>
#include <Spock/GhostPackage.h>
#include <Spock/Package.h>
#include <Spock/PackagePattern.h>

using namespace Spock;
using namespace Sawyer::Message::Common;

namespace {

Sawyer::Message::Facility mlog;
bool keepGoing = false;

std::vector<std::string>
parseCommandLine(int argc, char *argv[]) {
    using namespace Sawyer::CommandLine;
    Parser p = commandLineParser(purpose, description, mlog);
    p.doc("Synopsis", "@prop{programName} [@v{patterns}]");

    p.with(Switch("keep-going", 'k')
           .intrinsicValue(true, keepGoing)
           .doc("Don't stop if a download fails; try to download everything and report the number of failures at the end."));

    return p.parse(argc, argv).apply().unreachedArgs();
}

} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char *argv[]) {
    Spock::initialize(mlog);
    Sawyer::Message::mfacilities.control(">=march");
    std::vector<std::string> patterns = parseCommandLine(argc, argv);
    if (patterns.empty())
        patterns.push_back("");                         // matches everything

    size_t nErrors = 0;
    Spock::Context ctx;
    try {
        BOOST_FOREACH (const std::string &pattern, patterns) {
            std::vector<Package::Ptr> packages = ctx.findGhosts(pattern);
            if (packages.empty()) {
                ++nErrors;
                mlog[ERROR] <<"not found: " <<pattern <<"\n";
                if (!keepGoing)
                    break;
            } else {
                BOOST_FOREACH (const Package::Ptr &package, ctx.findGhosts(pattern)) {
                    VersionNumbers versions = package->versions();
                    BOOST_FOREACH (const VersionNumber &version, versions.values()) {
                        DefinedPackage::Settings dfSettings;
                        dfSettings.version = version;
                        dfSettings.quiet = !globalVerbose;
                        dfSettings.keepTempFiles = globalKeepTempFiles;
                        try {
                            std::cout <<asGhost(package)->definition()->download(ctx, dfSettings).string() <<"\n";
                        } catch (const Exception::SpockError &e) {
                            ++nErrors;
                            if (!keepGoing)
                                throw;
                        }
                    }
                }
            }
        }
    } catch (const Exception::SpockError &e) {
        mlog[ERROR] <<e.what() <<"\n";
        ++nErrors;
    }

    if (nErrors > 0 && keepGoing)
        mlog[ERROR] <<"download failed for " <<nErrors <<(1==nErrors?" file":" files") <<"\n";

    return nErrors ? 1 : 0;
}
