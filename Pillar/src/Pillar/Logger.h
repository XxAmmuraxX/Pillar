#pragma once
#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"

namespace Pillar
{
	class PIL_API Logger
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#define PIL_CORE_ERROR(...)    ::Pillar::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define PIL_CORE_WARN(...)     ::Pillar::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define PIL_CORE_INFO(...)     ::Pillar::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define PIL_CORE_TRACE(...)    ::Pillar::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define PIL_CORE_FATAL(...)    ::Pillar::Logger::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define PIL_ERROR(...)         ::Pillar::Logger::GetClientLogger()->error(__VA_ARGS__)
#define PIL_WARN(...)          ::Pillar::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define PIL_INFO(...)          ::Pillar::Logger::GetClientLogger()->info(__VA_ARGS__)
#define PIL_TRACE(...)         ::Pillar::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define PIL_FATAL(...)         ::Pillar::Logger::GetClientLogger()->fatal(__VA_ARGS__)

