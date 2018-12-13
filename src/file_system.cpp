#include "file_system.hpp"

#include <map>
#include <algorithm>

std::map<std::string, File*>* m_Files = nullptr;

File * FileSystem::GetFile(const std::string& a_FilePath)
{
	std::string t_FilePath = a_FilePath;
	std::transform(t_FilePath.begin(), t_FilePath.end(), t_FilePath.begin(), ::tolower);
	if (m_Files == nullptr)
	{
		m_Files = new std::map<std::string, File*>();
		std::atexit([]()
		{
			for (auto& i : *m_Files)
				delete i.second;

			m_Files->clear();
			m_Files = nullptr;
		});
	}

	auto* t_File = new File(t_FilePath);
	(*m_Files)[t_FilePath] = t_File;

	return t_File;
}

void FileSystem::CloseFile(File & a_File)
{
	auto t_FileName = a_File.GetName().c_str();
	auto t_Index = m_Files->find(a_File.GetName());
	if (t_Index == m_Files->end()) return;

	delete t_Index->second;
	m_Files->erase(t_Index);
}
