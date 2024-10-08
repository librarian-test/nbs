package common

import (
	"bytes"
	"hash/crc32"
)

////////////////////////////////////////////////////////////////////////////////

var zeroes = make([]byte, 1024*1024)

type Chunk struct {
	ID          string
	Index       uint32
	Data        []byte
	Zero        bool
	StoredInS3  bool
	Compression string
}

func (chunk Chunk) Checksum() uint32 {
	return crc32.ChecksumIEEE(chunk.Data)
}

func (chunk Chunk) CheckDataIsAllZeroes() bool {
	for i := 0; i < len(chunk.Data); i += len(zeroes) {
		endOffset := i + len(zeroes)

		if endOffset > len(chunk.Data) {
			endOffset = len(chunk.Data)
		}

		dataToCheck := chunk.Data[i:endOffset]

		if !bytes.Equal(dataToCheck, zeroes[:len(dataToCheck)]) {
			return false
		}
	}
	return true
}
