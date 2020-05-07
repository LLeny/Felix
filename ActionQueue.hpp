#pragma once

#include <cstdint>
#include <vector>

static constexpr uint64_t TICK_PERIOD_LOG = 5;
static constexpr uint64_t TICK_PERIOD = 1 << TICK_PERIOD_LOG;

enum class Action
{
  NONE,
  CPU_FETCH_OPCODE_RAM,
  CPU_FETCH_OPERAND_RAM,
  CPU_READ_RAM,
  CPU_WRITE_RAM,
  NONE_ROM,
  CPU_FETCH_OPCODE_ROM,
  CPU_FETCH_OPERAND_ROM,
  CPU_READ_ROM,
  CPU_WRITE_ROM,
  NONE_SUZY,
  CPU_FETCH_OPCODE_SUZY,
  CPU_FETCH_OPERAND_SUZY,
  CPU_READ_SUZY,
  CPU_WRITE_SUZY,
  NONE_MIKEY,
  CPU_FETCH_OPCODE_MIKEY,
  CPU_FETCH_OPERAND_MIKEY,
  CPU_READ_MIKEY,
  CPU_WRITE_MIKEY,
  END_FRAME,
  ACTIONS_END_
};

static_assert( (int)Action::ACTIONS_END_ <= TICK_PERIOD );

class SequencedAction
{
public:

  SequencedAction( Action action = Action::NONE, uint64_t tick = 0 );

  Action getAction() const;
  uint64_t getTick() const;

  explicit operator bool() const;

  friend bool operator<( SequencedAction left, SequencedAction right )
  {
    return left.mData > right.mData;
  }

private:
  uint64_t mData;
};

class ActionQueue
{
public:
  ActionQueue();


  void push( SequencedAction action );
  SequencedAction pop();

private:

  std::vector<SequencedAction> mHeap;
};

