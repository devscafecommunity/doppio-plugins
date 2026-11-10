#include "caffeine/plugin/PluginAPI.hpp"
#include "caffeine/plugin/PluginUI.hpp"
#include "caffeine/ui/UI.hpp"
#include "GitRunner.hpp"

#ifdef CF_HAS_IMGUI
#include <imgui.h>
#endif

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

class GitPlugin : public Caffeine::Editor::IPlugin {
public:
    explicit GitPlugin(const Caffeine::Editor::PluginHostApi* host) : m_host(host) {}

    void OnLoad() override {
        if (!m_host) return;
        Caffeine::Editor::PluginPanelInfo info;
        info.id = "plugin.git.panel";
        info.title = "Git";
        info.defaultOpen = false;
        info.icon = "git";
        info.dock = Caffeine::Editor::PluginDockArea::Right;
        info.size = 0.23f;
        Caffeine::Editor::RegisterPluginPanel(m_host, GetName(), info,
                                              &GitPlugin::renderPanel, this);
        if (m_host->registerMenuAction) {
            m_host->registerMenuAction(m_host->editorContext, GetName(), "Version Control/Git Panel",
                                       &GitPlugin::openPanelMenu, this);
        }
    }

    void OnUnload() override {}
    void OnUpdate(float) override {}

    const char* GetName() const override { return "Git"; }
    const char* GetVersion() const override { return "0.1.0"; }
    const char* GetDescription() const override {
        return "Optional Git project management — status, stage, commit, branches, remotes.";
    }

private:
    static void openPanelMenu(void* userData) {
        auto* self = static_cast<GitPlugin*>(userData);
        if (self->m_host && self->m_host->openPanel) {
            self->m_host->openPanel(self->m_host->editorContext, "Git");
        }
    }

    std::filesystem::path projectRoot() const {
        if (!m_host || !m_host->getProjectRootPath) return {};
        char buffer[1024] = {};
        if (!m_host->getProjectRootPath(m_host->editorContext, buffer, sizeof(buffer))) {
            return {};
        }
        return std::filesystem::path(buffer);
    }

    void refresh() {
        m_repoRoot = projectRoot();
        m_branch = Caffeine::Plugins::Git::currentBranch(m_repoRoot);
        m_files = Caffeine::Plugins::Git::listStatus(m_repoRoot);
        m_branches = Caffeine::Plugins::Git::listBranches(m_repoRoot);
        m_remotes = Caffeine::Plugins::Git::listRemotes(m_repoRoot);
        m_log = Caffeine::Plugins::Git::listLog(m_repoRoot, 40);
        m_lastOutput.clear();
    }

    void runGitCommand(const std::vector<std::string>& args) {
        const auto result = Caffeine::Plugins::Git::runGit(m_repoRoot, args);
        m_lastOutput = result.output;
        if (result.exitCode != 0 && m_host->logError) {
            m_host->logError(m_host->editorContext, result.output.c_str());
        } else if (m_host->logInfo) {
            m_host->logInfo(m_host->editorContext, "Git command completed.");
        }
        refresh();
    }

    static void renderPanel(void* userData) {
#ifdef CF_HAS_IMGUI
        auto* self = static_cast<GitPlugin*>(userData);
        using namespace Caffeine::UIKit;
        using namespace Caffeine::UIKit::Elements;

        if (self->m_needsRefresh) {
            self->refresh();
            self->m_needsRefresh = false;
        }

        UIContext ui;
        Column root("git_root");

        auto& header = root.emplace<Card>("git_header");
        header.emplace<Label>("Git", TextTone::Accent).setHeading(true);
        header.emplace<Label>("Version control", TextTone::Muted);
        header.emplace<Elements::Paragraph>(
            "Optional Git panel - not part of Doppio core. Requires git in PATH and "
            "an open project.");
        auto& refreshRow = header.emplace<Row>("git_refresh_row");
        auto& refresh = refreshRow.emplace<Button>("Refresh");
        refresh.setGrow(true);
        refresh.setTooltip("Reload git status");
        refresh.setOnClick([self] { self->refresh(); });

        if (self->m_repoRoot.empty()) {
            root.emplace<Elements::EmptyState>(
                "No repository", "Open a project scene to detect the repository root.",
                "git");
            ui.draw(root);
            return;
        }

        if (!Caffeine::Plugins::Git::isGitRepository(self->m_repoRoot)) {
            auto& warning = root.emplace<Card>("git_warning");
            warning.emplace<Label>("Not a git repository:", TextTone::Warning);
            warning.emplace<Label>(self->m_repoRoot.string(), TextTone::Muted).setWrap(true);
            auto& init = warning.emplace<Button>("git init");
            init.setTooltip("Initialise a repository here");
            init.setOnClick([self] { self->runGitCommand({"init"}); });
            ui.draw(root);
            return;
        }

        auto& info = root.emplace<Card>("git_info");
        info.emplace<Label>("Repo: " + self->m_repoRoot.string(), TextTone::Muted)
            .setWrap(true);
        info.emplace<Label>("Branch: " + (self->m_branch.empty() ? std::string("(detached)")
                                                                : self->m_branch),
                            TextTone::Normal);

        auto& changes = root.emplace<Elements::Section>("Changes");
        auto& changesRow = changes.emplace<Row>("git_changes_actions");
        changesRow.emplace<Button>("Stage all").setOnClick([self] {
            self->runGitCommand({"add", "-A"});
        });
        changesRow.emplace<Button>("Unstage all").setOnClick([self] {
            self->runGitCommand({"reset"});
        });

        auto& changeList = changes.emplace<ScrollArea>("git_changes");
        changeList.setHeight(160.0f);
        if (self->m_files.empty()) {
            changeList.emplace<Label>("Working tree clean.", TextTone::Muted);
        }
        for (std::size_t i = 0; i < self->m_files.size(); ++i) {
            const auto& file = self->m_files[i];
            auto& row = changeList.emplace<Row>("git_file_" + std::to_string(i));
            char tag[4] = {file.index, file.workTree, ' ', '\0'};
            auto& label = row.emplace<Label>(std::string(tag) + file.path, TextTone::Normal);
            label.setGrow(true);
            label.setTooltip(file.path);
            auto& stage = row.emplace<Button>("Stage");
            stage.setWidth(62.0f);
            stage.setOnClick([self, path = file.path] {
                self->runGitCommand({"add", path});
            });
            auto& diff = row.emplace<Button>("Diff");
            diff.setWidth(52.0f);
            diff.setOnClick([self, path = file.path] {
                self->m_lastOutput = Caffeine::Plugins::Git::runGit(
                                         self->m_repoRoot, {"diff", path})
                                         .output;
            });
        }

        auto& commit = root.emplace<Elements::Section>("Commit");
        auto& message = commit.emplace<MultilineText>(self->m_commitMessage,
                                                       sizeof(self->m_commitMessage));
        message.setHeight(60.0f);
        auto& commitRow = commit.emplace<Row>("git_commit_row");
        auto& commitButton = commitRow.emplace<Button>("Commit", ButtonVariant::Primary);
        commitButton.setGrow(true);
        commitButton.setOnClick([self] {
            if (self->m_commitMessage[0] == '\0') return;
            self->runGitCommand({"commit", "-m", self->m_commitMessage});
            self->m_commitMessage[0] = '\0';
        });

        auto& branches = root.emplace<Elements::Section>("Branches", false);
        if (!self->m_branches.empty()) {
            self->m_branchIndex = 0;
            for (std::size_t i = 0; i < self->m_branches.size(); ++i) {
                if (self->m_branches[i] == self->m_branch) {
                    self->m_branchIndex = static_cast<int>(i);
                }
            }
            auto& branchGrid = branches.emplace<PropertyGrid>("git_branches");
            branchGrid
                .typedRow<ChoiceProperty>("Checkout", self->m_branchIndex, self->m_branches)
                .setOnChange([self](int index) {
                    if (index >= 0 && index < static_cast<int>(self->m_branches.size())) {
                        self->runGitCommand({"checkout", self->m_branches[index]});
                    }
                });
        }
        auto& newBranchRow = branches.emplace<Row>("git_new_branch_row");
        auto& newBranch = newBranchRow.emplace<TextInput>(self->m_newBranch,
                                                           sizeof(self->m_newBranch));
        newBranch.setHint("New branch");
        newBranch.setGrow(true);
        auto& createBranch = newBranchRow.emplace<Button>("Create");
        createBranch.setWidth(78.0f);
        createBranch.setOnClick([self] {
            if (self->m_newBranch[0] == '\0') return;
            self->runGitCommand({"checkout", "-b", self->m_newBranch});
            self->m_newBranch[0] = '\0';
        });

        auto& remotes = root.emplace<Elements::Section>("Remotes & sync", false);
        if (self->m_remotes.empty()) {
            remotes.emplace<Label>("No remotes configured.", TextTone::Muted);
        }
        for (const auto& remote : self->m_remotes) {
            remotes.emplace<Label>("- " + remote, TextTone::Muted);
        }
        auto& remoteRow = remotes.emplace<Row>("git_remote_actions");
        remoteRow.emplace<Button>("Fetch").setOnClick([self] {
            self->runGitCommand({"fetch", "--all", "--prune"});
        });
        remoteRow.emplace<Button>("Pull").setOnClick([self] {
            self->runGitCommand({"pull"});
        });
        remoteRow.emplace<Button>("Push").setOnClick([self] {
            self->runGitCommand({"push"});
        });

        auto& history = root.emplace<Elements::Section>("History", false);
        auto& logList = history.emplace<ListBox>("git_log");
        logList.setHeight(120.0f);
        for (const auto& entry : self->m_log) {
            auto& item = logList.emplace<ListItem>(entry);
            item.setId("git_log_" + entry);
            item.setOnClick([self, entry] { self->m_lastOutput = entry; });
        }

        auto& stash = root.emplace<Elements::Section>("Stash", false);
        auto& stashRow = stash.emplace<Row>("git_stash_actions");
        stashRow.emplace<Button>("Stash push").setOnClick([self] {
            self->runGitCommand({"stash", "push", "-u"});
        });
        stashRow.emplace<Button>("Stash pop").setOnClick([self] {
            self->runGitCommand({"stash", "pop"});
        });
        stashRow.emplace<Button>("Stash list").setOnClick([self] {
            self->m_lastOutput =
                Caffeine::Plugins::Git::runGit(self->m_repoRoot, {"stash", "list"}).output;
        });

        if (!self->m_lastOutput.empty()) {
            auto& output = root.emplace<Card>("git_output_card");
            output.emplace<Label>("Output", TextTone::Accent).setHeading(true);
            auto& scroll = output.emplace<ScrollArea>("git_output");
            scroll.setHeight(120.0f);
            scroll.emplace<Label>(self->m_lastOutput, TextTone::Muted).setWrap(true);
        }

        ui.draw(root);
#else
        (void)userData;
#endif
    }

    const Caffeine::Editor::PluginHostApi* m_host = nullptr;
    std::filesystem::path m_repoRoot;
    std::string m_branch;
    std::vector<Caffeine::Plugins::Git::GitFileStatus> m_files;
    std::vector<std::string> m_branches;
    std::vector<std::string> m_remotes;
    std::vector<std::string> m_log;
    std::string m_lastOutput;
    char m_commitMessage[512] = {};
    char m_newBranch[128] = {};
    int m_branchIndex = 0;
    bool m_needsRefresh = true;
};

GitPlugin* g_plugin = nullptr;

}  // namespace

extern "C" {

CAFFEINE_PLUGIN_API Caffeine::Editor::IPlugin* CreatePlugin(
    const Caffeine::Editor::PluginHostApi* host) {
    g_plugin = new GitPlugin(host);
    return g_plugin;
}

CAFFEINE_PLUGIN_API void DestroyPlugin(Caffeine::Editor::IPlugin* plugin) {
    delete plugin;
    g_plugin = nullptr;
}

}  // extern "C"
