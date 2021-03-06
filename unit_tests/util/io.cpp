#include "util/io.hpp"
#include "storage/io.hpp"
#include "util/exception.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <exception>
#include <numeric>
#include <string>

const static std::string IO_TMP_FILE = "test_io.tmp";
const static std::string IO_NONEXISTENT_FILE = "non_existent_test_io.tmp";
const static std::string IO_TOO_SMALL_FILE = "file_too_small_test_io.tmp";
const static std::string IO_CORRUPT_FINGERPRINT_FILE = "corrupt_fingerprint_file_test_io.tmp";
const static std::string IO_TEXT_FILE = "plain_text_file.tmp";

BOOST_AUTO_TEST_SUITE(osrm_io)

BOOST_AUTO_TEST_CASE(io_data)
{
    std::vector<int> data_in(53), data_out;
    std::iota(begin(data_in), end(data_in), 0);

    osrm::util::serializeVector(IO_TMP_FILE, data_in);

    osrm::storage::io::FileReader infile(IO_TMP_FILE,
                                         osrm::storage::io::FileReader::VerifyFingerprint);
    infile.DeserializeVector(data_out);

    BOOST_REQUIRE_EQUAL(data_in.size(), data_out.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(data_out.begin(), data_out.end(), data_in.begin(), data_in.end());
}

BOOST_AUTO_TEST_CASE(io_nonexistent_file)
{
    try
    {
        osrm::storage::io::FileReader infile(IO_NONEXISTENT_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::exception &e)
    {
        const std::string expected("Error opening non_existent_test_io.tmp");
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
    }
}

BOOST_AUTO_TEST_CASE(file_too_small)
{
    {
        std::vector<int> v(53);
        std::iota(begin(v), end(v), 0);

        osrm::util::serializeVector(IO_TOO_SMALL_FILE, v);

        std::ofstream f(IO_TOO_SMALL_FILE);
        f.seekp(0, std::ios_base::beg);
        std::uint64_t garbage = 0xDEADBEEFCAFEFACE;
        f.write(reinterpret_cast<char *>(&garbage), sizeof(garbage));
    }

    try
    {
        osrm::storage::io::FileReader infile(IO_TOO_SMALL_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::exception &e)
    {
        const std::string expected(
            "Error reading from file_too_small_test_io.tmp: Unexpected end of file");
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
    }
}

BOOST_AUTO_TEST_CASE(io_corrupt_fingerprint)
{
    {
        std::vector<int> v(153);
        std::iota(begin(v), end(v), 0);
        osrm::util::serializeVector(IO_CORRUPT_FINGERPRINT_FILE, v);

        std::fstream f(IO_CORRUPT_FINGERPRINT_FILE);
        f.seekp(0, std::ios_base::beg);
        std::uint64_t garbage = 0xDEADBEEFCAFEFACE;
        f.write(reinterpret_cast<char *>(&garbage), sizeof(garbage));
    }

    try
    {
        osrm::storage::io::FileReader infile(IO_CORRUPT_FINGERPRINT_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::exception &e)
    {
        const std::string expected("Fingerprint mismatch in corrupt_fingerprint_file_test_io.tmp");
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
    }
}

BOOST_AUTO_TEST_CASE(io_read_lines)
{
    {
        std::ofstream f(IO_TEXT_FILE, std::ios::binary);
        char str[] = "A\nB\nC\nD";
        f.write(str, strlen(str));
    }
    {
        osrm::storage::io::FileReader infile(IO_TEXT_FILE,
                                             osrm::storage::io::FileReader::HasNoFingerprint);
        auto startiter = infile.GetLineIteratorBegin();
        auto enditer = infile.GetLineIteratorEnd();
        std::vector<std::string> resultlines;
        while (startiter != enditer)
        {
            resultlines.push_back(*startiter);
            ++startiter;
        }
        BOOST_REQUIRE_MESSAGE(resultlines.size() == 4, "Expected 4 lines of text");
        BOOST_REQUIRE_MESSAGE(resultlines[0] == "A", "Expected the first line to be A");
        BOOST_REQUIRE_MESSAGE(resultlines[1] == "B", "Expected the first line to be B");
        BOOST_REQUIRE_MESSAGE(resultlines[2] == "C", "Expected the first line to be C");
        BOOST_REQUIRE_MESSAGE(resultlines[3] == "D", "Expected the first line to be D");
    }
}

BOOST_AUTO_TEST_SUITE_END()
