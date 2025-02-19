#pragma once

#include <nano/lib/blocks.hpp>
#include <nano/lib/logging.hpp>
#include <nano/node/blocking_observer.hpp>
#include <nano/secure/common.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <thread>

namespace nano::store
{
class write_transaction;
}

namespace nano
{
class node;
class write_database_queue;

/**
 * Processing blocks is a potentially long IO operation.
 * This class isolates block insertion from other operations like servicing network operations
 */
class block_processor final
{
public:
	explicit block_processor (nano::node &, nano::write_database_queue &);
	void stop ();
	void flush ();
	std::size_t size ();
	bool full ();
	bool half_full ();
	void add (std::shared_ptr<nano::block> const &);
	std::optional<nano::process_return> add_blocking (std::shared_ptr<nano::block> const & block);
	void force (std::shared_ptr<nano::block> const &);
	bool should_log ();
	bool have_blocks_ready ();
	bool have_blocks ();
	void process_blocks ();

	std::atomic<bool> flushing{ false };

public: // Events
	using processed_t = std::pair<nano::process_return, std::shared_ptr<nano::block>>;
	nano::observer_set<nano::process_return const &, std::shared_ptr<nano::block>> processed;

	// The batch observer feeds the processed obsever
	nano::observer_set<std::deque<processed_t> const &> batch_processed;

private:
	blocking_observer blocking;

private:
	// Roll back block in the ledger that conflicts with 'block'
	void rollback_competitor (store::write_transaction const & transaction, nano::block const & block);
	nano::process_return process_one (store::write_transaction const &, std::shared_ptr<nano::block> block, bool const = false);
	void queue_unchecked (store::write_transaction const &, nano::hash_or_account const &);
	std::deque<processed_t> process_batch (nano::unique_lock<nano::mutex> &);
	void add_impl (std::shared_ptr<nano::block> block);
	bool stopped{ false };
	bool active{ false };
	std::chrono::steady_clock::time_point next_log;
	std::deque<std::shared_ptr<nano::block>> blocks;
	std::deque<std::shared_ptr<nano::block>> forced;
	nano::condition_variable condition;
	nano::node & node;
	nano::write_database_queue & write_database_queue;
	nano::mutex mutex{ mutex_identifier (mutexes::block_processor) };
	std::thread processing_thread;

	friend std::unique_ptr<container_info_component> collect_container_info (block_processor & block_processor, std::string const & name);
};
std::unique_ptr<nano::container_info_component> collect_container_info (block_processor & block_processor, std::string const & name);
}
