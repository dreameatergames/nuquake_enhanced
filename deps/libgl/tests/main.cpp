

#include <functional>
#include <memory>
#include <map>

#include "tools/test.h"

#include "/home/dreamdev/GLdc/tests/test_glteximage2d.h"
#include "/home/dreamdev/GLdc/tests/test_allocator.h"


std::map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::map<std::string, std::string> ret;

    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto eq = arg.find('=');
        if(eq != std::string::npos && arg[0] == '-' && arg[1] == '-') {
            auto key = std::string(arg.begin(), arg.begin() + eq);
            auto value = std::string(arg.begin() + eq + 1, arg.end());
            ret[key] = value;
        } else if(arg[0] == '-' && arg[1] == '-') {
            auto key = arg;
            if(i < (argc - 1)) {
                auto value = argv[++i];
                ret[key] = value;
            } else {
                ret[key] = "";
            }
        } else {
            ret[arg] = "";  // Positional, not key=value
        }
    }

    return ret;
}

int main(int argc, char* argv[]) {
    auto runner = std::make_shared<test::TestRunner>();

    auto args = parse_args(argc, argv);

    std::string junit_xml;
    auto junit_xml_it = args.find("--junit-xml");
    if(junit_xml_it != args.end()) {
        junit_xml = junit_xml_it->second;
        std::cout << "    Outputting junit XML to: " << junit_xml << std::endl;
        args.erase(junit_xml_it);
    }

    std::string test_case;
    if(args.size()) {
        test_case = args.begin()->first;
    }

    
    runner->register_case<AllocatorTests>(
        std::vector<void (AllocatorTests::*)()>({&AllocatorTests::test_defrag, &AllocatorTests::test_poor_alloc_aligned, &AllocatorTests::test_poor_alloc_straddling, &AllocatorTests::test_alloc_init, &AllocatorTests::test_complex_case, &AllocatorTests::test_complex_case2, &AllocatorTests::test_alloc_malloc}),
        {"AllocatorTests::test_defrag", "AllocatorTests::test_poor_alloc_aligned", "AllocatorTests::test_poor_alloc_straddling", "AllocatorTests::test_alloc_init", "AllocatorTests::test_complex_case", "AllocatorTests::test_complex_case2", "AllocatorTests::test_alloc_malloc"}
    );

    runner->register_case<TexImage2DTests>(
        std::vector<void (TexImage2DTests::*)()>({&TexImage2DTests::test_rgb_to_rgb565, &TexImage2DTests::test_rgb_to_rgb565_twiddle, &TexImage2DTests::test_rgba_to_argb4444, &TexImage2DTests::test_rgba_to_argb4444_twiddle}),
        {"TexImage2DTests::test_rgb_to_rgb565", "TexImage2DTests::test_rgb_to_rgb565_twiddle", "TexImage2DTests::test_rgba_to_argb4444", "TexImage2DTests::test_rgba_to_argb4444_twiddle"}
    );

    return runner->run(test_case, junit_xml);
}


