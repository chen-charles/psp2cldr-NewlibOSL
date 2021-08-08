#include <psp2cldr/imp_provider.hpp>

#if defined(_MSC_VER) || (__GNUC__ >= 8)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <iostream>
#include <sstream>
#include <string>

#include <atomic>
#include <fstream>
#include <regex>
#include <unordered_map>

#include "target.hpp"

std::unordered_map<uint32_t, std::pair<std::string, std::shared_ptr<std::fstream>>> file_mapping;
std::atomic<uint32_t> fd_ctr{3};

static std::string translate_path(std::string name, LoadContext &load)
{
    // fs::path filename{name};
    // auto root_name = filename.root_name(); // app0 savedata0 etc.

    // fs::path velf_path{ctx->load.main_velf_fullname};
    // auto translated_filename = velf_path.parent_path() / filename.relative_path();

    fs::path velf_path{load.main_velf_fullname};
    return (velf_path.parent_path() / std::regex_replace(name, std::regex(":"), "")).string();
}

#undef _fstat
DEFINE_VITA_IMP_SYM_EXPORT(_fstat)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t fd = PARAM_0;
    uint32_t buf = PARAM_1;

    if (file_mapping.count(fd) == 0)
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
        TARGET_RETURN(-1);
        HANDLER_RETURN(0);
    }
    else
    {
        auto filename = file_mapping.at(fd).first;
        target::stat s;
        memset(&s, 0, sizeof(s));
        s.st_size = sizeof(s);
        if (fs::is_directory(filename))
        {
            s.st_mode |= 0100000;
        }
        else
        {
            s.st_mode |= 0040000;
        }
        ctx->coord.proxy().copy_in(buf, &s, sizeof(s));
        TARGET_RETURN(0);
    }
    HANDLER_RETURN(0);
}

#undef _close
DEFINE_VITA_IMP_SYM_EXPORT(_close)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t fd = PARAM_0;
    if (file_mapping.count(fd) != 0)
    {
        file_mapping.erase(fd);
        ctx->thread[RegisterAccessProxy::Register::R0]->w(0);
    }
    else
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _isatty
DEFINE_VITA_IMP_SYM_EXPORT(_isatty)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t fd = PARAM_0;
    if (fd >= 0 && fd <= 2)
        ctx->thread[RegisterAccessProxy::Register::R0]->w(1);
    else
        ctx->thread[RegisterAccessProxy::Register::R0]->w(0);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _write
DEFINE_VITA_IMP_SYM_EXPORT(_write)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t fd = PARAM_0;
    uint32_t ptr = PARAM_1;
    uint32_t len = PARAM_2;

    std::unique_ptr<char[]> buf(new char[len + 1]);
    memset(buf.get(), 0, len + 1);
    ctx->coord.proxy().copy_out(buf.get(), ptr, len);

    switch (fd)
    {
    case 0:
        // ?
        std::cout << "_write() called on stdin: " << buf.get();
        break;
    case 1:
        std::cout << /* "stdout: " << */ buf.get();
        break;
    case 2:
        std::cerr << /* "stderr: " << */ buf.get();
        break;
    default:
    {
        if (file_mapping.count(fd) != 0)
        {
            auto file = file_mapping.at(fd).second;
            ctx->coord.proxy().copy_out(buf.get(), ptr, len);
            file->write(buf.get(), len);
            ctx->thread[RegisterAccessProxy::Register::R0]->w(len);
        }
        else
            ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }
    }

    TARGET_RETURN(len);
    HANDLER_RETURN(0);
}

#undef _open
DEFINE_VITA_IMP_SYM_EXPORT(_open)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t name = PARAM_0;
    uint32_t flags = PARAM_1;
    uint32_t mode = PARAM_2;

    auto filename_cstr = ctx->read_str(name);
    auto translated = translate_path(filename_cstr, ctx->load);

    if (!fs::exists(translated))
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }
    else
    {
        std::ios::openmode ios_mode = std::ios_base::binary;

        if (flags & 2)
            ios_mode |= std::ios_base::in | std::ios_base::out;
        else if (flags & 1)
            ios_mode |= std::ios_base::out;
        else
            ios_mode |= std::ios_base::in;

        if (flags & 8)
            ios_mode |= std::ios_base::app;

        if (flags & 0x0100)
            std::ofstream _(translated);

        if (flags & 0x0200 && ios_mode & std::ios_base::out)
            ios_mode |= std::ios_base::trunc;

        auto stream = std::make_shared<std::fstream>(translated, ios_mode);
        if (stream->is_open())
        {
            auto fd_out = fd_ctr++;
            file_mapping[fd_out] = std::make_pair(translated, stream);
            ctx->thread[RegisterAccessProxy::Register::R0]->w(fd_out);
        }
    }

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _read
DEFINE_VITA_IMP_SYM_EXPORT(_read)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t fd = PARAM_0;
    uint32_t ptr = PARAM_1;
    uint32_t len = PARAM_2;

    if (file_mapping.count(fd) != 0)
    {
        auto file = file_mapping.at(fd).second;
        std::unique_ptr<char[]> buffer(new char[len]);
        file->read(buffer.get(), len);
        file->clear();
        auto n_read = file->gcount();
        ctx->coord.proxy().copy_in(ptr, buffer.get(), n_read);
        ctx->thread[RegisterAccessProxy::Register::R0]->w(n_read);
    }
    else
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _lseek
DEFINE_VITA_IMP_SYM_EXPORT(_lseek)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t fd = PARAM_0;
    int32_t offset = PARAM_1;
    uint32_t whence = PARAM_2;

    if (file_mapping.count(fd) != 0)
    {
        auto file = file_mapping.at(fd).second;
        file->clear();
        switch (whence)
        {
        case SEEK_SET:
            file->seekg(offset, std::ios_base::beg);
            break;
        case SEEK_CUR:
            file->seekg(offset, std::ios_base::cur);
            break;
        case SEEK_END:
            file->seekg(offset, std::ios_base::end);
            break;
        }
        file->clear();
        ctx->thread[RegisterAccessProxy::Register::R0]->w(file->tellg());
    }
    else
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _link
DEFINE_VITA_IMP_SYM_EXPORT(_link)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t old_ = PARAM_0;
    uint32_t new_ = PARAM_1;

    auto translated_old = translate_path(ctx->read_str(old_), ctx->load);
    auto translated_new = translate_path(ctx->read_str(new_), ctx->load);

    if (!fs::exists(translated_old) || fs::exists(translated_new))
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }

    fs::create_hard_link(translated_old, translated_new);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(0);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _unlink
DEFINE_VITA_IMP_SYM_EXPORT(_unlink)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}
