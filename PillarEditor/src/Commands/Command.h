#pragma once

#include <string>
#include <memory>

namespace PillarEditor {

    /**
     * @brief Base class for all undoable commands
     * 
     * Commands encapsulate editor operations that can be undone/redone.
     * Each command stores the state needed to reverse its operation.
     */
    class Command
    {
    public:
        virtual ~Command() = default;

        /**
         * @brief Execute the command
         * Called when the command is first performed or redone
         */
        virtual void Execute() = 0;

        /**
         * @brief Undo the command
         * Reverse the operation performed by Execute()
         */
        virtual void Undo() = 0;

        /**
         * @brief Get human-readable name of the command
         * Used in UI (e.g., "Undo Move Entity")
         */
        virtual std::string GetName() const = 0;

        /**
         * @brief Check if command is valid
         * Commands may become invalid if entities are deleted externally
         */
        virtual bool IsValid() const { return true; }
    };

} // namespace PillarEditor
