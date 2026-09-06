#include "tool_registry.h"

namespace tools {

const std::vector<ToolDef>& Registry() {
    static const std::vector<ToolDef> registry = {
        BuildTimestampTool(),
        BuildBase64Tool(),
        BuildHashTool(),
        BuildMd5Tool(),
        BuildSha1Tool(),
        BuildSha256Tool(),
        BuildUrlTool(),
        BuildUuidTool(),
    };
    return registry;
}

const ToolDef* FindByKeyword(const std::string& keyword) {
    for (const auto& tool : Registry()) {
        if (keyword == tool.keyword) {
            return &tool;
        }
    }
    return nullptr;
}

} // namespace tools
