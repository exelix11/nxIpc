#pragma once
#include <switch.h>
#include <cstring>
#include <algorithm>

namespace nxIpc
{
	struct PACKED IPCServerHeader {
		u64 magic;
		union
		{
			u64 cmdId;
			u64 result;
		};
	};

	struct Buffer
	{
		const void* data;
		size_t length;

		inline void CopyTo(void* src)
		{
			std::memcpy(src, data, length);
		}

		inline void CopyTo_s(void* src, size_t size)
		{
			std::memcpy(src, data, std::min(size, length));
		}
	};

	struct WritableBuffer
	{
		void* data;
		size_t length;

		inline void AssignFrom(const void* src)
		{
			std::memcpy(data, src, length);
		}

		inline void AssignFrom_s(const void* src, size_t size)
		{
			std::memcpy(data, src, std::min(size, length));
		}
	};

	struct IPCRequest
	{
		IPCRequest(IPCRequest&) = delete;
		IPCRequest& operator=(IPCRequest&) = delete;

		HipcParsedRequest hipc;
		u64 cmdId = 0;
		u32 size = 0;
		void* ptr = nullptr;

		template <typename T> const T* Payload()
		{
			if (sizeof(T) > size)
				throw std::logic_error("Payload size is wrong");

			return (T*)ptr;
		}

		Buffer ReadBuffer(u8 index)
		{
			if (index >= hipc.meta.num_send_buffers)
				throw std::logic_error("Read buffer index out of bounds");

			Buffer res = { hipcGetBufferAddress(&hipc.data.send_buffers[index]), hipcGetBufferSize(&hipc.data.send_buffers[index]) };

			if (!res.data)
				throw std::runtime_error("Read buffer pointer is null");

			return res;
		}

		WritableBuffer WriteBuffer(u8 index)
		{
			if (index >= hipc.meta.num_recv_buffers)
				throw std::logic_error("Write buffer index out of bounds");

			WritableBuffer res = { hipcGetBufferAddress(&hipc.data.recv_buffers[index]), hipcGetBufferSize(&hipc.data.recv_buffers[index]) };

			if (!res.data)
				throw std::runtime_error("Write buffer pointer is null");

			return res;
		}

		static IPCRequest ParseFromTLS()
		{
			return IPCRequest();
		}

	private:
		IPCRequest()
		{
			void* base = armGetTls();

			hipc = hipcParseRequest(base);

			if (hipc.meta.type == CmifCommandType_Request)
			{
				IPCServerHeader* header = (IPCServerHeader*)cmifGetAlignedDataStart(hipc.data.data_words, base);
				size_t dataSize = hipc.meta.num_data_words * 4;

				if (!header)
					throw std::runtime_error("HeaderPTR is null");
				if (dataSize < sizeof(IPCServerHeader))
					throw std::runtime_error("Data size is smaller than sizeof(IPCServerHeader)");
				if (header->magic != CMIF_IN_HEADER_MAGIC)
					throw std::runtime_error("Header magic is wrong");

				cmdId = header->cmdId;
				size = dataSize - sizeof(IPCServerHeader);
				if (size)
					ptr = ((u8*)header) + sizeof(IPCServerHeader);
			}
		}
	};

	struct IPCResponse
	{
		template<typename T>
		IPCResponse& Payload(const T& p)
		{
			if (R_FAILED(result))
				throw std::logic_error("Payload is not supported when result is non zero");

			payload = { &p, sizeof(T) };
			return *this;
		}

		IPCResponse(Result rc = 0)
		{
			result = rc;
		}

		IPCResponse& CopyHandle(Handle h)
		{
			if (R_FAILED(result))
				throw std::logic_error("handles are not supported when result is non zero");

			meta.num_copy_handles = 1;
			copyHandle = h;
			return *this;
		}

		Result PrepareResponse()
		{
			meta.type = CmifCommandType_Request;
			meta.num_data_words = (sizeof(IPCServerHeader) + payload.length + 0x10) / 4;

			void* base = armGetTls();
			HipcRequest hipc = hipcMakeRequest(base, meta);
			IPCServerHeader* rawHeader = (IPCServerHeader*)cmifGetAlignedDataStart(hipc.data_words, base);

			rawHeader->magic = CMIF_OUT_HEADER_MAGIC;
			rawHeader->result = result;

			if (payload.length)
				std::memcpy(((u8*)rawHeader) + sizeof(IPCServerHeader), payload.data, payload.length);

			if (meta.num_copy_handles)
				hipc.copy_handles[0] = copyHandle;

			return result;
		}

	protected:
		HipcMetadata meta = { 0 };

		Buffer payload = { 0 };
		Handle copyHandle;

		Result result;
	};
}