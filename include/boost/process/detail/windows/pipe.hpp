// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_PIPE_HPP
#define BOOST_PROCESS_WINDOWS_PIPE_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/pipes.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/detail/winapi/file_management.hpp>
#include <boost/detail/winapi/get_last_error.hpp>
#include <boost/detail/winapi/access_rights.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <system_error>
#include <array>

namespace boost { namespace process { namespace detail { namespace windows {



class pipe
{
    boost::detail::winapi::HANDLE_ _source;
    boost::detail::winapi::HANDLE_ _sink;
protected:
    pipe(boost::detail::winapi::HANDLE_ source, boost::detail::winapi::HANDLE_ sink) : _source(source), _sink(sink) {}
public:
    pipe() : pipe(create()) {}
    pipe(const pipe& ) = delete;
    pipe(pipe&& lhs)  : _source(lhs._source), _sink(lhs._sink)
    {
        lhs._source = nullptr;
        lhs._sink = nullptr;
    }
    pipe& operator=(const pipe& ) = delete;
    pipe& operator=(pipe&& lhs)
    {
        lhs._source = _source;
        lhs._sink   = _sink;
        _source = nullptr;
        _sink   = nullptr;
        return *this;
    }
    ~pipe()
    {
        if (_sink   != nullptr)
            boost::detail::winapi::CloseHandle(_sink);
        if (_source != nullptr)
            boost::detail::winapi::CloseHandle(_source);
    }
    void * source() const {return _source;}
    void * sink  () const {return _sink;}

    static pipe create()
    {
        boost::detail::winapi::HANDLE_ handles[2];
        if (!::boost::detail::winapi::CreatePipe(&handles[0], &handles[1], nullptr, 0))
            throw std::system_error(
                    std::error_code(
                    ::boost::detail::winapi::GetLastError(),
                    std::system_category()),
                    "CreatePipe() failed");
        return pipe(handles[0], handles[1]);
    }

    static pipe create(std::error_code &ec)
    {
        boost::detail::winapi::HANDLE_ handles[2];
        if (!::boost::detail::winapi::CreatePipe(&handles[0], &handles[1], nullptr, 0))
            ec = std::error_code(
                ::boost::detail::winapi::GetLastError(),
                std::system_category());
        else
            ec.clear();
        return pipe(handles[0], handles[1]);
    }
    inline static std::string make_pipe_name();

    inline static pipe create_named(const std::string & name = make_pipe_name());
    inline static pipe create_named(const std::string & name, std::error_code & ec);
    inline static pipe create_named(std::error_code & ec)
    {
        return create_named(make_pipe_name(), ec);
    }

    inline static pipe create_async()                     {return create_named();  }
    inline static pipe create_async(std::error_code & ec) {return create_named(ec);}

};

std::string pipe::make_pipe_name()
{
    std::string name = "\\\\.\\pipe\\boost_process_auto_pipe_";

    namespace fs = boost::filesystem;
    fs::path p;

    do
    {
        static unsigned long long int i = 0;
        p = name + std::to_string(i);
        i++;
    }
    while (fs::exists(p)); //so it limits it to 2^31 pipes. should suffice.
    return p.string();
}



pipe pipe::create_named(const std::string & name)
{
    static constexpr int OPEN_EXISTING_         = 3; //temporary.
    static constexpr int FILE_FLAG_OVERLAPPED_  = 0x40000000; //temporary
    //static constexpr int FILE_ATTRIBUTE_NORMAL_ = 0x00000080; //temporary

    boost::detail::winapi::HANDLE_ handle1 = boost::detail::winapi::create_named_pipe(
            name.c_str(),
            boost::detail::winapi::PIPE_ACCESS_INBOUND_
            | FILE_FLAG_OVERLAPPED_, //write flag
            0, 1, 8192, 8192, 0, nullptr);

    if (handle1 == boost::detail::winapi::INVALID_HANDLE_VALUE_)
        boost::process::detail::throw_last_error("create_named_pipe() failed");

    boost::detail::winapi::HANDLE_ handle2 = boost::detail::winapi::create_file(
            name.c_str(),
            boost::detail::winapi::GENERIC_WRITE_, 0, nullptr,
            OPEN_EXISTING_,
            FILE_FLAG_OVERLAPPED_, //to allow read
            nullptr);

    if (handle2 == boost::detail::winapi::INVALID_HANDLE_VALUE_)
        boost::process::detail::throw_last_error("create_file() failed");

    return pipe(handle1, handle2);
}

pipe pipe::create_named(const std::string & name, std::error_code & ec)
{
    static constexpr int OPEN_EXISTING_         = 3; //temporary.
    static constexpr int FILE_FLAG_OVERLAPPED_  = 0x40000000; //temporary
    //static constexpr int FILE_ATTRIBUTE_NORMAL_ = 0x00000080; //temporary

    boost::detail::winapi::HANDLE_ handle1 = boost::detail::winapi::create_named_pipe(
            name.c_str(),
            boost::detail::winapi::PIPE_ACCESS_INBOUND_
            | FILE_FLAG_OVERLAPPED_, //write flag
            0, 1, 8192, 8192, 0, nullptr);

    if (handle1 == boost::detail::winapi::INVALID_HANDLE_VALUE_)
        ec = boost::process::detail::get_last_error();

    boost::detail::winapi::HANDLE_ handle2;

    if (!ec)
    {
        handle2 = boost::detail::winapi::create_file(
                name.c_str(),
                boost::detail::winapi::GENERIC_WRITE_, 0, nullptr,
                OPEN_EXISTING_,
                FILE_FLAG_OVERLAPPED_, //to allow read
                nullptr);

        if (handle2 == boost::detail::winapi::INVALID_HANDLE_VALUE_)
            ec = boost::process::detail::get_last_error();
    }

    return pipe(handle1, handle2);
}

typedef boost::asio::windows::stream_handle async_pipe_handle;


}}}}

#endif