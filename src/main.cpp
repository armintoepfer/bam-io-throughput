#include <pbbam/BamReader.h>
#include <pbbam/BamWriter.h>
#include <pbbam/DataSet.h>
#include <pbbam/EntireFileQuery.h>
#include <pbbam/FastaWriter.h>
#include <pbbam/PbiFilterQuery.h>
#include <pbbam/PbiRawData.h>
#include <pbcopper/cli2/CLI.h>
#include <pbcopper/cli2/internal/BuiltinOptions.h>
#include <pbcopper/logging/Logging.h>
#include <pbcopper/parallel/WorkQueue.h>
#include <pbcopper/utility/MemoryConsumption.h>
#include <pbcopper/utility/Ssize.h>
#include <pbcopper/utility/Stopwatch.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace PacBio {
namespace OptionNames {
// clang-format off
const CLI_v2::Option CompressionThreads {
R"({
    "names" : ["compression-threads"],
    "description" : "BAM compression threads",
    "type" : "int",
    "default" : 1
})"
};
const CLI_v2::Option DecompressionThreads {
R"({
    "names" : ["decompression-threads"],
    "description" : "BAM compression threads",
    "type" : "int",
    "default" : 1
})"
};
// clang-format on
}  // namespace OptionNames

constexpr char BamReaderEnv[] = "PB_BAMREADER_THREADS";

struct DaiDaiSettings
{
    std::string InputFile;
    std::string OutputFile;
    int32_t NumThreads = 1;
    int32_t CompressionThreads = 1;
    int32_t DecompressionThreads = 1;
};

CLI_v2::Interface CreateCLI()
{
    static const std::string description{"BAM I/O test."};
    const auto version = "0.0.1";
    CLI_v2::Interface i{"biot", description, version};

    Logging::LogConfig logConfig;
    logConfig.Header = "| ";
    logConfig.Delimiter = " | ";
    logConfig.Fields = Logging::LogField::TIMESTAMP | Logging::LogField::LOG_LEVEL;
    i.LogConfig(logConfig);

    const CLI_v2::PositionalArgument InpuFile{
        R"({
        "name" : "IN.bam",
        "description" : "Input BAM.",
        "type" : "file",
        "required" : true
    })"};
    const CLI_v2::PositionalArgument Output{
        R"({
        "name" : "OUT.bam",
        "description" : "Output BAM.",
        "type" : "file",
        "required" : true
    })"};
    i.AddPositionalArguments({InpuFile, Output});
    i.AddOption(OptionNames::CompressionThreads);
    i.AddOption(OptionNames::DecompressionThreads);

    return i;
}

void WorkerThread(Parallel::WorkQueue<std::vector<BAM::BamRecord>>& queue, BAM::BamWriter& writer,
                  const int32_t numReads)
{
    int32_t counter = 0;
    double perc = 0.1;

    auto LambdaWorker = [&](std::vector<BAM::BamRecord>&& ps) {
        ++counter;
        if (1.0 * counter / numReads > perc) {
            PBLOG_INFO << "Progress " << 100 * perc << '%';
            perc = std::min(perc + 0.1, 1.0);
        }
        for (const auto& record : ps) {
            writer.Write(record);
        }
    };

    while (queue.ConsumeWith(LambdaWorker)) {
    }
}

int RunnerSubroutine(const CLI_v2::Results& options)
{
    Utility::Stopwatch globalTimer;

    DaiDaiSettings settings;
    settings.CompressionThreads = options[OptionNames::CompressionThreads];
    settings.DecompressionThreads = options[OptionNames::DecompressionThreads];
    const std::vector<std::string> files = options.PositionalArguments();
    settings.InputFile = files[0];
    settings.OutputFile = files[1];

    if (settings.DecompressionThreads > 1) {
        std::string DecompString = std::to_string(settings.DecompressionThreads);
        setenv(BamReaderEnv, DecompString.c_str(), true);
    }

    BAM::BamReader reader(settings.InputFile);
    BAM::BamHeader header = reader.Header().DeepCopy();
    BAM::BamWriter writer(settings.OutputFile, header, BAM::BamWriter::DefaultCompression,
                          settings.CompressionThreads);

    int32_t reads = 0;
    int64_t yield = 0;
    for (auto& read : reader) {
        ++reads;
        yield += read.Impl().SequenceLength();
        writer.Write(read);
    }

    globalTimer.Freeze();

    const auto yieldToString = [](const int64_t yield) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        if (yield >= 1000000000000) {
            ss << (yield / 1000000000000.0) << " TBases";
        } else if (yield >= 1000000000) {
            ss << (yield / 1000000000.0) << " GBases";
        } else if (yield >= 1000000) {
            ss << (yield / 1000000.0) << " MBases";
        } else if (yield >= 1000) {
            ss << (yield / 1000.0) << " KBases";
        } else {
            ss << yield << " Bases";
        }
        return ss.str();
    };

    const double yieldPerMinute = 60 * 1000.0 * yield / globalTimer.ElapsedMilliseconds();
    PBLOG_INFO << "Reads      : " << reads;
    PBLOG_INFO << "Yield      : " << yieldToString(yield);
    PBLOG_INFO << "Throughput : " << yieldToString(yieldPerMinute) << "/min";
    PBLOG_INFO << "Run Time   : " << globalTimer.ElapsedTime();
    PBLOG_INFO << "CPU Time   : "
               << Utility::Stopwatch::PrettyPrintNanoseconds(
                      static_cast<int64_t>(Utility::Stopwatch::CpuTime() * 1000 * 1000 * 1000));

    int64_t const peakRss = PacBio::Utility::MemoryConsumption::PeakRss();
    double const peakRssGb = peakRss / 1024.0 / 1024.0 / 1024.0;
    PBLOG_INFO << "Peak RSS   : " << std::fixed << std::setprecision(3) << peakRssGb << " GB";

    return EXIT_SUCCESS;
}
}  // namespace PacBio

int main(int argc, char* argv[])
{
    return PacBio::CLI_v2::Run(argc, argv, PacBio::CreateCLI(), &PacBio::RunnerSubroutine);
}
