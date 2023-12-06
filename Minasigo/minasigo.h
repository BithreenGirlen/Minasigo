#ifndef MINASIGO_H_
#define MINASIGO_H_

#include <string>

namespace minasigo
{
	bool GetStoryMasterData();
	void GetScenarioPaths();
	void DownloadScenarios();
	void GetScenarioResourcePaths();
	void DownloadScenarioResources();

	const std::wstring GetToken();
	void MnsgSetup(void* arg);
}

#endif //MINASIGO_H_
