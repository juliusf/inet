//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "inet/common/packet/Packet.h"

namespace inet {

Register_Class(Packet);

Packet::Packet(const char *name, short kind) :
    cPacket(name, kind),
    data(std::make_shared<SequenceChunk>()),
    headerIterator(data->createForwardIterator()),
    trailerIterator(data->createBackwardIterator())
{
}

Packet::Packet(const Packet& other) :
    cPacket(other),
    data(other.isImmutable() ? other.data : std::make_shared<SequenceChunk>(*other.data)),
    headerIterator(other.headerIterator),
    trailerIterator(other.trailerIterator)
{
}

int Packet::getNumChunks() const
{
    return data->getChunks().size();
}

Chunk *Packet::getChunk(int i) const
{
    return data->getChunks()[i].get();
}

std::shared_ptr<Chunk> Packet::peekHeader(int64_t byteLength) const
{
    return data->peek(headerIterator, byteLength);
}

std::shared_ptr<Chunk> Packet::peekHeaderAt(int64_t byteOffset, int64_t byteLength) const
{
    return data->peek(SequenceChunk::SequenceIterator(true, -1, byteOffset), byteLength);
}

std::shared_ptr<Chunk> Packet::popHeader(int64_t byteLength)
{
    const auto& chunk = peekHeader(byteLength);
    if (chunk != nullptr)
        headerIterator.move(data, chunk->getByteLength());
    return chunk;
}

std::shared_ptr<Chunk> Packet::peekTrailer(int64_t byteLength) const
{
    return data->peek(trailerIterator, byteLength);
}

std::shared_ptr<Chunk> Packet::peekTrailerAt(int64_t byteOffset, int64_t byteLength) const
{
    return data->peek(SequenceChunk::SequenceIterator(false, -1, byteOffset), byteLength);
}

std::shared_ptr<Chunk> Packet::popTrailer(int64_t byteLength)
{
    const auto& chunk = peekTrailer(byteLength);
    if (chunk != nullptr)
        trailerIterator.move(data, -chunk->getByteLength());
    return chunk;
}

std::shared_ptr<Chunk> Packet::peekData(int64_t byteLength) const
{
    int64_t peekByteLength = byteLength == -1 ? getDataLength() : byteLength;
    return data->peek(SequenceChunk::SequenceIterator(true, -1, getDataPosition()), peekByteLength);
}

std::shared_ptr<Chunk> Packet::peekDataAt(int64_t byteOffset, int64_t byteLength) const
{
    int64_t peekByteOffset = getDataPosition() + byteOffset;
    int64_t peekByteLength = byteLength == -1 ? getDataLength() - byteOffset : byteLength;
    return data->peek(SequenceChunk::SequenceIterator(true, -1, peekByteOffset), peekByteLength);
}

std::shared_ptr<Chunk> Packet::peek(int64_t byteLength) const
{
    int64_t peekByteLength = byteLength == -1 ? getByteLength() : byteLength;
    return data->peek(SequenceChunk::SequenceIterator(true, -1, 0), peekByteLength);
}

std::shared_ptr<Chunk> Packet::peekAt(int64_t byteOffset, int64_t byteLength) const
{
    int64_t peekByteLength = byteLength == -1 ? getByteLength() - byteOffset : byteLength;
    return data->peek(SequenceChunk::SequenceIterator(true, -1, byteOffset), peekByteLength);
}

void Packet::prepend(const std::shared_ptr<Chunk>& chunk, bool flatten)
{
    data->prepend(chunk, flatten);
}

void Packet::prepend(Packet* packet, bool flatten)
{
    data->prepend(packet->data, flatten);
}

void Packet::append(const std::shared_ptr<Chunk>& chunk, bool flatten)
{
    data->append(chunk, flatten);
}

void Packet::append(Packet* packet, bool flatten)
{
    data->append(packet->data, flatten);
}

} // namespace