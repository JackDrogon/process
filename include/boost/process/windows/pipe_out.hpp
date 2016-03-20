// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_INITIALIZERS_PIPE_OUT_HPP
#define BOOST_PROCESS_WINDOWS_INITIALIZERS_PIPE_OUT_HPP

#include <boost/detail/winapi/process.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

template<int p1, int p2>
struct pipe_out : public ::boost::process::detail::handler_base
{
    boost::iostreams::file_descriptor_source file;

    pipe_out(FILE * f) : file(_get_osfhandle(_fileno(f)), boost::iostreams::never_close_handle) {}
    pipe_out(const pipe & p) : file(p.source(), boost::iostreams::never_close_handle) {}
    pipe_out(const boost::iostreams::file_descriptor_source &f) : file(f, boost::iostreams::never_close_handle) {}

    template <typename WindowsExecutor>
    void on_setup(WindowsExecutor &e) const;
};

template<>
template<typename WindowsExecutor>
void pipe_out<1,0>::on_setup<WindowsExecutor>(WindowsExecutor &e) const
{
    e.startup_info.hStdOutput = file.handle();
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
}

template<>
template<typename WindowsExecutor>
void pipe_out<2,0>::on_setup<WindowsExecutor>(WindowsExecutor &e) const
{
    e.startup_info.hStdError = file.handle();
    e.startup_info.dwFlags  |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
}

template<>
template<typename WindowsExecutor>
void pipe_out<1,2>::on_setup<WindowsExecutor>(WindowsExecutor &e) const
{
    e.startup_info.hStdOutput = file.handle();
    e.startup_info.hStdError  = file.handle();
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
}

}}}}

#endif