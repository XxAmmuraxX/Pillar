#pragma once

#include "Command.h"
#include "../EditorConstants.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

namespace PillarEditor {

    using namespace Constants;

    /**
     * @brief Manages command history for undo/redo functionality
     * 
     * Maintains two stacks: undo stack (past commands) and redo stack (undone commands).
     * When a new command is executed, it's added to undo stack and redo stack is cleared.
     */
    class CommandHistory
    {
    public:
        using OnCommandExecutedCallback = std::function<void()>;

        CommandHistory(size_t maxHistorySize = Performance::MAX_UNDO_HISTORY)
            : m_MaxHistorySize(maxHistorySize)
        {
        }

        /**
         * @brief Execute a command and add it to history
         * Clears the redo stack since we're creating a new timeline
         */
        void ExecuteCommand(std::unique_ptr<Command> command)
        {
            if (!command)
                return;

            // Execute the command
            command->Execute();

            // Add to undo stack
            m_UndoStack.push_back(std::move(command));

            // Clear redo stack (new action creates new timeline)
            m_RedoStack.clear();

            // Limit history size
            if (m_UndoStack.size() > m_MaxHistorySize)
            {
                m_UndoStack.erase(m_UndoStack.begin());
            }

            // Notify callback
            if (m_OnCommandExecuted)
                m_OnCommandExecuted();
        }

        /**
         * @brief Undo the last command
         * @return true if undo was performed
         */
        bool Undo()
        {
            if (m_UndoStack.empty())
                return false;

            // Get last command
            auto command = std::move(m_UndoStack.back());
            m_UndoStack.pop_back();

            // Check if still valid
            if (!command->IsValid())
            {
                // Skip invalid commands
                return Undo();
            }

            // Undo it
            command->Undo();

            // Move to redo stack
            m_RedoStack.push_back(std::move(command));

            // Notify callback
            if (m_OnCommandExecuted)
                m_OnCommandExecuted();

            return true;
        }

        /**
         * @brief Redo the last undone command
         * @return true if redo was performed
         */
        bool Redo()
        {
            if (m_RedoStack.empty())
                return false;

            // Get last undone command
            auto command = std::move(m_RedoStack.back());
            m_RedoStack.pop_back();

            // Check if still valid
            if (!command->IsValid())
            {
                // Skip invalid commands
                return Redo();
            }

            // Re-execute it
            command->Execute();

            // Move back to undo stack
            m_UndoStack.push_back(std::move(command));

            // Notify callback
            if (m_OnCommandExecuted)
                m_OnCommandExecuted();

            return true;
        }

        /**
         * @brief Check if undo is available
         */
        bool CanUndo() const
        {
            return !m_UndoStack.empty();
        }

        /**
         * @brief Check if redo is available
         */
        bool CanRedo() const
        {
            return !m_RedoStack.empty();
        }

        /**
         * @brief Get name of command that would be undone
         */
        std::string GetUndoName() const
        {
            if (m_UndoStack.empty())
                return "";
            return m_UndoStack.back()->GetName();
        }

        /**
         * @brief Get name of command that would be redone
         */
        std::string GetRedoName() const
        {
            if (m_RedoStack.empty())
                return "";
            return m_RedoStack.back()->GetName();
        }

        /**
         * @brief Clear all history
         */
        void Clear()
        {
            m_UndoStack.clear();
            m_RedoStack.clear();
        }

        /**
         * @brief Get size of undo stack
         */
        size_t GetUndoStackSize() const { return m_UndoStack.size(); }

        /**
         * @brief Get size of redo stack
         */
        size_t GetRedoStackSize() const { return m_RedoStack.size(); }

        /**
         * @brief Set callback to be called when commands are executed/undone/redone
         */
        void SetOnCommandExecuted(OnCommandExecutedCallback callback)
        {
            m_OnCommandExecuted = callback;
        }

    private:
        std::vector<std::unique_ptr<Command>> m_UndoStack;
        std::vector<std::unique_ptr<Command>> m_RedoStack;
        size_t m_MaxHistorySize;
        OnCommandExecutedCallback m_OnCommandExecuted;
    };

} // namespace PillarEditor
