#include "GLCtx.h"

#include "GL.h"

bool Ren::ReadbackTimestampQueries(const ApiContext &api, int i) {
    const uint32_t query_count = api.query_counts[i];
    if (!query_count) {
        // nothing to readback
        return true;
    }

    for (uint32_t j = 0; j < query_count; ++j) {
        glGetQueryObjectui64v(api.queries[i][j], GL_QUERY_RESULT, &api.query_results[i][j]);
    }
    api.query_counts[i] = 0;

    return true;
}