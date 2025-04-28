module;

#include "HydraEngine/Base.h"
#include "git2.h"

export module Git;

import HE;
import std;

export namespace Git {

	enum class CloneState
	{
		None,
		Cloning,
		Completed,
		Canceled,
		Faild
	};

	struct ProgressInfo
	{
		git_indexer_progress fetchProgress;
		size_t completedSteps;
		size_t totalSteps;
		std::string stepName;

		CloneState cloneState;
	};
	
	using ::git_repository;
}

// Internal
namespace Git {

	int FetchProgress(const git_indexer_progress* stats, void* payload)
	{
		ProgressInfo* progress = (ProgressInfo*)payload;
		progress->fetchProgress = *stats;

		if (progress->cloneState == CloneState::Canceled)
			return -1;

		return 0;
	}

	void CheckoutProgress(const char* path, size_t cur, size_t tot, void* payload)
	{
		ProgressInfo* progress = (ProgressInfo*)payload;
		//if (path)
		//{
		//	HE_INFO("({}/{}) : {}", tot, cur, path);
		//}
	}
}

export namespace Git {

	void Init() 
	{
		git_libgit2_init(); 
	}
	void Shutdown() 
	{
		git_libgit2_shutdown();
	}

	int GetProgress(const ProgressInfo& progress)
	{
		int network_percent = progress.fetchProgress.total_objects > 0 ? (100 * progress.fetchProgress.received_objects) / progress.fetchProgress.total_objects : 0;
		if (progress.fetchProgress.total_objects && progress.fetchProgress.received_objects == progress.fetchProgress.total_objects)
		{
			return 0;
		}
		else
		{
			return network_percent;
		}
	}

	int GetSubmodulesCount(git_repository* repo)
	{
		int total = 0;

		int state = git_submodule_foreach(repo, [](git_submodule* sm, const char* name, void* payload) -> int 
		{
			int* total = static_cast<int*>(payload);
			(*total)++; 
			return 0;
		}, 
		&total
		);

		return total;
	}

	int IntrnalUpdateAllSubmodules(git_repository* repo, ProgressInfo* progress)
	{
		return git_submodule_foreach(repo, [](git_submodule* sm, const char* name, void* payload) -> int {
			ProgressInfo* progress = static_cast<ProgressInfo*>(payload);
			progress->cloneState = CloneState::Cloning;

			git_submodule_update_options opts = GIT_SUBMODULE_UPDATE_OPTIONS_INIT;
			opts.fetch_opts.depth = 1;
			opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
			opts.checkout_opts.progress_cb = CheckoutProgress;
			opts.checkout_opts.progress_payload = progress;
			opts.fetch_opts.callbacks.transfer_progress = &FetchProgress;
			opts.fetch_opts.callbacks.payload = progress;

			HE_INFO("Updating submodule: {}", name);
			progress->completedSteps++;
			progress->stepName = name;

			progress->fetchProgress = { 0 };
			int err = git_submodule_update(sm, 1, &opts);

			// Recurse into nested submodules
			git_repository* sub_repo = nullptr;
			if (git_submodule_open(&sub_repo, sm) == 0 && sub_repo)
			{
				progress->totalSteps += GetSubmodulesCount(sub_repo);

				err |= IntrnalUpdateAllSubmodules(sub_repo, progress);
				git_repository_free(sub_repo);
			}

			return err;
		}, progress);
	}

	int UpdateAllSubmodules(git_repository* clonedRepo, ProgressInfo& progress)
	{
		HE_ASSERT(clonedRepo);
		int err = -1;

		err = IntrnalUpdateAllSubmodules(clonedRepo, &progress);
		if (err != 0)
		{
			progress.cloneState = CloneState::None;
			HE_ERROR("clone not complet : {}", err);
		}

		return err;
	}

	void FreeRepo(git_repository* repo)
	{
		HE_ASSERT(repo);
		git_repository_free(repo);
		repo = nullptr;
	}

	void Cancel(ProgressInfo& progress) { progress.cloneState = CloneState::Canceled; }

	git_repository* Clone(const char* url, const char* path, ProgressInfo& progress, int& err)
	{
		git_remote* remote = nullptr;
		git_repository* repo = NULL;
		git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
		git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

		clone_opts.fetch_opts.depth = 1;
		checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
		checkout_opts.progress_cb = CheckoutProgress;
		checkout_opts.progress_payload = &progress;
		clone_opts.checkout_opts = checkout_opts;
		clone_opts.fetch_opts.callbacks.transfer_progress = &FetchProgress;
		clone_opts.fetch_opts.callbacks.payload = &progress;
		progress.cloneState = CloneState::Cloning;

		HE_INFO("Clone : {}", path);
		progress.completedSteps++;
		err = git_clone(&repo, url, path, &clone_opts);

		if (err != 0)
		{
			progress.cloneState = CloneState::None;

			const git_error* e = git_error_last();
			if (e)
			{
				HE_ERROR("{} : {}", e->message, e->klass);
			}
			else
			{
				HE_ERROR("{} : no detailed info", err);
			}
		}

		return repo;
	}

	CloneState RecursiveClone(const char* url, const char* path, ProgressInfo& progress)
	{
		progress.cloneState = CloneState::Cloning;

		int err = -1;
		git_repository* repo = Clone(url, path, progress, err);

		if (repo && err == 0)
		{
			progress.totalSteps += GetSubmodulesCount(repo);

			err = UpdateAllSubmodules(repo, progress);
		}

		if (repo)
		{
			git_repository_free(repo);
			repo = nullptr;
		}

		return err == 0 ? CloneState::Completed : CloneState::Faild;
	}

	std::string GetCurrentCommitId(const std::filesystem::path& repoPath)
	{
		git_repository* repo = nullptr;
		auto fileStr = repoPath.string();
		if (git_repository_open(&repo, fileStr.c_str()) != 0)
		{
			HE_ERROR("Failed to open Git repository at: ", fileStr);
			return {};
		}

		git_reference* headRef = nullptr;
		if (git_repository_head(&headRef, repo) != 0) {
			HE_ERROR("Failed to get HEAD reference");
			git_repository_free(repo);
			return {};
		}

		const git_oid* oid = nullptr;

		if (git_reference_type(headRef) == GIT_REFERENCE_DIRECT)
		{
			oid = git_reference_target(headRef);
		}
		else if (git_reference_type(headRef) == GIT_REFERENCE_SYMBOLIC) 
		{
			git_reference* resolvedRef = nullptr;
			if (git_reference_resolve(&resolvedRef, headRef) != 0)
			{
				HE_ERROR("Failed to resolve symbolic HEAD");

				git_reference_free(headRef);
				git_repository_free(repo);
				return {};
			}
			oid = git_reference_target(resolvedRef);
			git_reference_free(resolvedRef);
		}

		if (!oid) 
		{
			HE_ERROR("Failed to get commit OID from HEAD");

			git_reference_free(headRef);
			git_repository_free(repo);
			return {};
		}

		char oidStr[GIT_OID_HEXSZ + 1];
		git_oid_tostr(oidStr, sizeof(oidStr), oid);

		git_reference_free(headRef);
		git_repository_free(repo);

		return std::string(oidStr,7);
	}
}