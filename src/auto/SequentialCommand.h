#ifndef AUTO_SEQUENTIAL_COMMAND_H_
#define AUTO_SEQUENTIAL_COMMAND_H_

#include <vector>
#include <utility>

#include "auto/AutoCommand.h"

/**
 * @author Eric Bakan
 *
 * Meta-command used for running multiple commands sequentially
 */
class SequentialCommand : public AutoCommand {
 public:
  /**
   * Constructor
   * Accepts a variable-length list of AutoCommand pointers to execute sequentially
   * The commands will be executed in the order they are passed into the constructor
   * @param numCommands the number of commands to execute
   * @param ... the commands to run
   */
  SequentialCommand(int numCommands, ...);

  /**
   * Destructor
   * Deallocates the AutoCommands stored in commands_
   */
  virtual ~SequentialCommand();

  /**
   * Calls Initialize() on the first AutoCommand in commands_
   */
  virtual void Initialize();

  /**
   * Calls Run() on the current command
   * If the current command is complete, calls Initialize() on the next one
   * @return true if the last command is complete, else false
   */
  virtual bool Run();
 private:
  std::vector<AutoCommand*> commands_;
  int commandIndex_; // the index of the current command being executed
};

#endif  // AUTO_SEQUENTIAL_COMMAND_H_
