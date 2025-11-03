#pragma once
#include <vector>
#include <algorithm>

using namespace std;

class RingBuffer
{

public:
	RingBuffer(const RingBuffer&) = delete;  
	RingBuffer& operator=(const RingBuffer&) = delete;

	RingBuffer()
		: buffer(8192)
		, head(0)
		, tail(0)
		, dataSize(0)
	{
	}

	RingBuffer(size_t cap)
		: buffer(cap)
		, head(0)
		, tail(0)
		, dataSize(0)
	{
	}

	RingBuffer(RingBuffer&&) noexcept = default;
	RingBuffer& operator=(RingBuffer&&) noexcept = default;
	~RingBuffer() = default;

	bool Write(const char* data, size_t len)
	{
		if (GetFreeSize() < len)
		{
			return false;
		}

		size_t capacity = buffer.size();

		size_t firstPart = min(len, capacity - head);

		memcpy(buffer.data() + head, data, firstPart);

		if (len > firstPart)
		{
			memcpy(buffer.data(), data + firstPart, len - firstPart);
		}

		head = (head + len) % capacity;
		dataSize += len;
		return true;
	}

	bool Peek(char* outBuffer, size_t len)
	{
		if (dataSize < len)
		{
			return false;
		}

		size_t capacity = buffer.size();
		size_t firstPart = min(len, capacity - tail);

		memcpy(outBuffer, buffer.data() + tail, firstPart);

		if (len > firstPart)
		{
			memcpy(outBuffer + firstPart, buffer.data(), len - firstPart);
		}

		return true;
	}

	void Consume(size_t len)
	{
		if (len > dataSize)
		{
			len = dataSize;
		}

		tail = (tail + len) % buffer.size();
		dataSize -= len;
	}

	size_t GetFreeSize() const { return buffer.size() - dataSize - 1; }
	size_t GetDataSize() const { return dataSize; }
	bool IsEmpty() const { return dataSize == 0; }
	bool IsFull() const { return buffer.size() >= dataSize - 1; }

private:
	vector<char> buffer;
	size_t head;
	size_t tail;
	size_t dataSize;
};