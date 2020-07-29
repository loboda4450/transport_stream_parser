#include <iostream>
#include <cstdio>

using namespace std;

class xTS {
    public:
        static constexpr uint32_t TS_PacketLength = 188;
        static constexpr uint32_t TS_HeaderLength = 4;
        static constexpr uint32_t PES_HeaderLength = 6;
        static constexpr uint32_t BaseToExtendedClockMultiplier =      300;
};

class xTS_PacketHeader {
    protected:
        uint8_t syncByte;                   //8b
        bool transportErrorIndicator;       //1b
        bool payloadUnitStartIndicator;     //1b
        bool transportPriority;             //1b
        uint16_t packetIdentifier;          //13b
        uint8_t transportScramblingControl; //2b
        uint8_t adaptationFieldControl;     //2b
        uint8_t continuityCounter;          //4b
                                            //32b = 4B

    public:
        void Reset() {
            syncByte = 0;
            transportErrorIndicator = false;
            payloadUnitStartIndicator = false;
            transportPriority = false;
            packetIdentifier = 0;
            transportScramblingControl = 0;
            adaptationFieldControl = 0;
            continuityCounter = 0;
        };

        int32_t Parse(const uint8_t *Input) {
            syncByte = Input[0];
            transportErrorIndicator = (Input[1] & 0b10000000) != 0;
            payloadUnitStartIndicator = (Input[1] & 0b01000000) != 0;
            transportPriority = (Input[1] & 0b00100000) != 0;
            packetIdentifier = ((((uint16_t)(Input[1]) << 8)) | (uint16_t)(Input[2])) & 8191; //binary 0b0001111111111111
            transportScramblingControl = Input[3] & 0b11000000;
            adaptationFieldControl = (Input[3] & 0b00110000) >> 4;
            continuityCounter = Input[3] & 0b00001111;

            return 0;
        };

        void Print() const {
            printf("SB=%2d E=%d S=%d P=%d PID=%4d TSC=%d AF=%d CC=%2d",
                   syncByte,transportErrorIndicator, payloadUnitStartIndicator,
                   transportPriority, packetIdentifier, transportScramblingControl,
                   adaptationFieldControl, continuityCounter);
        };

    public:
        uint8_t getSyncByte() const { return syncByte; }
        bool isPayloadUnitStartIndicator() const { return payloadUnitStartIndicator; }
        uint16_t getPacketIdentifier() const { return packetIdentifier; }
        uint8_t getAdaptationFieldControl() const { return adaptationFieldControl; }

    public:
        bool hasAdaptationField() const { return adaptationFieldControl == 2 or adaptationFieldControl == 3; }
        bool hasPayload() const { return adaptationFieldControl == 1 or adaptationFieldControl == 3; }
};

class xTS_AdaptationField {
    protected:
        uint8_t  adaptationFieldLength;                     //8b
        bool  discontinuityIndicator;                       //1b
        bool  randomAccessIndicator;                        //1b
        bool  elementaryStreamPriorityIndicator;            //1b
        bool  programClockReferenceFlag;                    //1b
        bool  originalProgramClockReferenceFlag;            //1b
        bool  splicingPointFlag;                            //1b
        bool  transportPrivateDataFlag;                     //1b
        bool  adaptationFieldExtensionFlag;                 //1b
        uint64_t programClockReferenceBase;                 //33b
        uint8_t PCRReserved;                                //6b
        uint16_t programClockReferenceExtension;            //9b
        uint64_t originalProgramClockReferenceBase;         //33b
        uint8_t OPCRReserved;                               //6b
        uint16_t originalProgramClockReferenceExtension;    //9b
        uint8_t  spliceCountdown;                           //8b
        uint8_t  transportPrivateDataLength;                //1b
        uint8_t  adaptationFieldExtensionLength;            //8b
        uint8_t  stuffingLength;
                                                            //adaptationFieldLength + 1B

    public:
        void Reset(){
            adaptationFieldLength = 0;
            discontinuityIndicator = false;
            randomAccessIndicator = false;
            elementaryStreamPriorityIndicator = false;
            programClockReferenceFlag = false;
            originalProgramClockReferenceFlag = false;
            splicingPointFlag = false;
            transportPrivateDataFlag = false;
            adaptationFieldExtensionFlag = false;

            programClockReferenceBase = 0;
            PCRReserved = 0;
            programClockReferenceExtension = 0;

            programClockReferenceBase = 0;
            PCRReserved = 0;
            programClockReferenceExtension = 0;

            adaptationFieldExtensionLength = 0;
        }

        int32_t Parse(const uint8_t *Input, uint8_t AdaptationFieldControl) {
            if (AdaptationFieldControl == 2 || AdaptationFieldControl == 3) {
                adaptationFieldLength = Input[4] + 1;
                if (adaptationFieldLength > 1) {
                    discontinuityIndicator = (Input[5] & 0b10000000) == 128;
                    randomAccessIndicator = (Input[5] & 0b01000000) == 64;
                    elementaryStreamPriorityIndicator = (Input[5] & 0b00100000) == 32;
                    programClockReferenceFlag = (Input[5] & 0b00010000) == 16;
                    originalProgramClockReferenceFlag = (Input[5] & 0b00001000) == 8;
                    splicingPointFlag = (Input[5] & 0b00000100) == 4;
                    transportPrivateDataFlag = (Input[5] & 0b00000010) == 2;
                    adaptationFieldExtensionFlag = (Input[5] & 0b00000001) == 1;
                    stuffingLength = adaptationFieldLength - 2;

                    if (programClockReferenceFlag) {
                        programClockReferenceBase = ((uint64_t) Input[6]) << 25 | ((uint64_t) Input[7]) << 17 |
                                                    ((uint64_t) Input[8]) << 9 | ((uint64_t) Input[9]) << 1 |
                                                    ((uint64_t) Input[10]) >> 7;
                        PCRReserved = Input[10] & 0b01111110;
                        programClockReferenceExtension =
                                ((uint16_t) (Input[10] & 0b00000001)) << 8 | ((uint16_t) (Input[11]));
                        stuffingLength -= 6;
                    }

                    if (originalProgramClockReferenceFlag) {
                        originalProgramClockReferenceExtension =
                                Input[12] | Input[13] | Input[14] | Input[15] | (Input[16] & 0b10000000);
                        OPCRReserved = Input[16] & 0b01111110;
                        originalProgramClockReferenceExtension = (Input[16] & 0b00000001) | Input[17];
                        stuffingLength -= 6;
                    }

                    if (splicingPointFlag) stuffingLength--;
                    if (transportPrivateDataFlag) stuffingLength--;
                    if (adaptationFieldExtensionFlag) {
                        adaptationFieldExtensionLength = Input[20];
                        stuffingLength -= 2;
                        if (Input[21] & 0b10000000) stuffingLength -= 2;
                        if (Input[21] & 0b01000000) stuffingLength -= 3;
                        if (Input[21] & 0b00100000) stuffingLength--;
                    }
                }
            } else if (AdaptationFieldControl == 1){
                adaptationFieldLength = 0;
            }
            return 0;
        }

        void Print() const {
            printf(" AF: L=%3d DC=%d RA=%d SP=%d PR=%d OR=%d SP=%d TP=%d EX=%d",
                   adaptationFieldLength - 1,
                   discontinuityIndicator,
                   randomAccessIndicator,
                   elementaryStreamPriorityIndicator,
                   programClockReferenceFlag,
                   originalProgramClockReferenceFlag,
                   splicingPointFlag,
                   transportPrivateDataFlag,
                   adaptationFieldExtensionFlag);

            if(programClockReferenceFlag){
                printf(" PCR=%lu", programClockReferenceBase*xTS::BaseToExtendedClockMultiplier + programClockReferenceExtension);
            }

            if(originalProgramClockReferenceFlag){
                printf(" OPCR=%lu", originalProgramClockReferenceBase*xTS::BaseToExtendedClockMultiplier + originalProgramClockReferenceExtension);
            }

            if(adaptationFieldExtensionFlag){
                printf(" AFexLen=%d", adaptationFieldExtensionLength);
            }

            printf(" Stuffing=%d", stuffingLength);
        }

        uint32_t getNumBytes() const { return adaptationFieldLength; }
};

class xPES_PacketHeader {
    public:
        enum eStreamId : uint8_t {
            eStreamId_program_stream_map = 0xBC,
            eStreamId_padding_stream = 0xBE,
            eStreamId_private_stream_2 = 0xBF,
            eStreamId_ECM = 0xF0,
            eStreamId_EMM = 0xF1,
            eStreamId_program_stream_directory = 0xFF,
            eStreamId_DSMCC_stream = 0xF2,
            eStreamId_ITUT_H222_1_type_E = 0xF8,
        };

    protected:
        //PES packet header
        uint32_t m_PacketStartCodePrefix;
        uint8_t m_StreamId;
        uint16_t m_PacketLength;
        uint8_t PESScramblingControl;
        bool PESPriority;
        bool DataAlignmentIndicator;
        bool Copyright;
        bool OriginalOrCopy;
        uint8_t PTSDTSFlags;
        bool ESCRFlag;
        bool ESRateFlag;
        bool DSMTrickModeFlag;
        bool AdditionalCopyInfoFlag;
        bool PESCRCFlag;
        bool PESExtensionFlag;
        uint8_t PESHeaderDataLength;
        uint64_t PTS;
        uint64_t DTS;

    public:
        void Reset() {
            m_PacketStartCodePrefix = 0;
            m_StreamId = 0;
            m_PacketLength = 0;
            PESScramblingControl = 0;
            PESPriority = false;
            DataAlignmentIndicator = false;
            Copyright = false;
            OriginalOrCopy = false;
            PTSDTSFlags = 0;
            ESCRFlag = false;
            ESRateFlag = false;
            DSMTrickModeFlag = false;
            AdditionalCopyInfoFlag = false;
            PESCRCFlag = false;
            PESExtensionFlag = false;
            PESHeaderDataLength = xTS::PES_HeaderLength;
            PTS = 0;
            DTS = 0;
        };

        int32_t Parse(const uint8_t *Input, uint8_t AdaptationFieldOffset = 0) {
            m_PacketStartCodePrefix = ((uint32_t) (Input[AdaptationFieldOffset]) << 16 | (uint32_t) (Input[AdaptationFieldOffset + 1]) << 8 |
                    (uint32_t) (Input[AdaptationFieldOffset + 2]));
            m_StreamId = Input[AdaptationFieldOffset + 3];
            m_PacketLength = (uint16_t) Input[AdaptationFieldOffset + 4] << 8 | (uint16_t) Input[AdaptationFieldOffset + 5];
            if ((Input[AdaptationFieldOffset + 6] & 0b11000000) == 128) {
                if (m_StreamId != eStreamId::eStreamId_program_stream_map
                    and m_StreamId != eStreamId::eStreamId_padding_stream
                    and m_StreamId != eStreamId::eStreamId_private_stream_2
                    and m_StreamId != eStreamId::eStreamId_ECM
                    and m_StreamId != eStreamId::eStreamId_EMM
                    and m_StreamId != eStreamId::eStreamId_program_stream_directory
                    and m_StreamId != eStreamId::eStreamId_DSMCC_stream
                    and m_StreamId != eStreamId::eStreamId_ITUT_H222_1_type_E) {
                    PESScramblingControl = (Input[AdaptationFieldOffset + 6] & 0b00110000) >> 4;
                    PESPriority = (Input[AdaptationFieldOffset + 6] & 0b0001000) != 0;
                    DataAlignmentIndicator = (Input[AdaptationFieldOffset + 6] & 0b00000100) != 0;
                    Copyright = (Input[AdaptationFieldOffset + 6] & 0b00000010) != 0;
                    OriginalOrCopy = (Input[AdaptationFieldOffset + 6] & 0b00000001) != 0;
                    PTSDTSFlags = (Input[AdaptationFieldOffset + 7] & 0b11000000) >> 6;
                    ESCRFlag = (Input[AdaptationFieldOffset + 7] & 0b00100000) != 0;
                    ESRateFlag = (Input[AdaptationFieldOffset + 7] & 0b00010000) != 0;
                    DSMTrickModeFlag = (Input[AdaptationFieldOffset + 7] & 0b00001000) != 0;
                    AdditionalCopyInfoFlag = (Input[AdaptationFieldOffset + 7] & 0b00000100) != 0;
                    PESCRCFlag = (Input[AdaptationFieldOffset + 7] & 0b00000010) != 0;
                    PESExtensionFlag = (Input[AdaptationFieldOffset + 7] & 0b00000001) != 0;
                    PESHeaderDataLength += Input[AdaptationFieldOffset + 8] + 3;
                    if ((PTSDTSFlags & 0b00000010) != 0 and (Input[AdaptationFieldOffset + 9] & 0b11110000) == 32) {
                        PTS = (((uint64_t)(Input[AdaptationFieldOffset + 9] & 0b00001110)) << 29 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 10])) << 21 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 11] & 0b11111110)) << 14 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 12])) << 7 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 13] & 0b11111110)) >> 1);

                    }

                    if ((PTSDTSFlags & 0b00000001) != 0 and (Input[AdaptationFieldOffset + 9] & 0b11110000) == 48) {
                        DTS = (((uint64_t)(Input[AdaptationFieldOffset + 14] & 0b00001110)) << 29 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 15])) << 21 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 16] & 0b11111110)) << 14 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 17])) << 7 |
                                ((uint64_t)(Input[AdaptationFieldOffset + 18] & 0b11111110)) >> 1);
                    }
                }
                return AdaptationFieldOffset + PESHeaderDataLength;
            }
            return AdaptationFieldOffset + xTS::PES_HeaderLength;
        };

        void Print() const {
            printf(" PSCP=%1d SID=%3d L=%4d PL=%4d HL=%d DL=%4d", m_PacketStartCodePrefix, m_StreamId, m_PacketLength,
                   (m_PacketLength == 0 ? 0 : m_PacketLength + xTS::PES_HeaderLength), PESHeaderDataLength,
                   (m_PacketLength == 0 ? 0 : getPacketLength() - getPesHeaderDataLength()));
            if((PTSDTSFlags & 0b00000010) != 0) {
                printf(" PTS=%lu", PTS);
            }

            if((PTSDTSFlags & 0b00000001) != 0){
                printf(" DTS=%lu", DTS);
            }
        };

    public:
        //PES packet header
        uint16_t getPacketLength() const { return m_PacketLength + xTS::PES_HeaderLength; }
        uint8_t getPesHeaderDataLength() const { return PESHeaderDataLength; }
};


class xPES_Assembler {
public:
    enum class eResult : int32_t {
        UnexpectedPID = 1,
        StreamPacketLost,
        AssemblingStarted,
        AssemblingContinue,
        AssemblingFinished,
    };

protected:
    //setup
    int32_t m_PID;
    FILE *ofs = nullptr;
    //buffer
    uint8_t *m_Buffer = nullptr;
    uint8_t *m_TmpBuffer = nullptr;
    uint32_t m_BufferSize;
    uint8_t m_pesOffset;
    uint32_t m_DataOffset;
    //operation
    uint8_t m_LastContinuityCounter;
    bool m_Started = false;
    xPES_PacketHeader m_PESH;

public:
    xPES_Assembler() {};

    ~xPES_Assembler() {};

    void Init(int32_t PID) {
        m_PID = PID;
        if (m_PID == 136) ofs = fopen("/home/loobson/Politechnika/rewrite/pid136.mp2", "wb");
        else if (m_PID == 174) ofs = fopen("/home/loobson/Politechnika/rewrite/pid174.264", "wb");
    };

    eResult AbsorbPacket(const uint8_t *TransportStreamPacket, const xTS_PacketHeader *PacketHeader,
                         const xTS_AdaptationField *AdaptationField) {
        if (PacketHeader->getPacketIdentifier() == m_PID) {
            if (PacketHeader->isPayloadUnitStartIndicator()) {
                if (m_Started) {
                    m_Started = false;
                    fwrite(m_Buffer, m_BufferSize, 1, ofs);
                    printf(" PES: Finished with previous, Len=%4d ", m_BufferSize);
                }

                if (!m_Started) {
                    m_Started = true;
                    xBufferReset();
                    m_pesOffset = m_PESH.Parse(TransportStreamPacket,
                                                xTS::TS_HeaderLength + AdaptationField->getNumBytes());
                    if(m_PID != 174) m_BufferSize = m_PESH.getPacketLength() - m_PESH.getPesHeaderDataLength();
                    xBufferAppend(TransportStreamPacket, m_pesOffset);
                    return eResult::AssemblingStarted;
                }
            } else {
                if (PacketHeader->hasPayload()) {
                    xBufferAppend(TransportStreamPacket, xTS::TS_HeaderLength + AdaptationField->getNumBytes());
                    m_LastContinuityCounter++;
                }

                return eResult::AssemblingContinue;
            }
        } else return eResult::UnexpectedPID;

        return eResult::AssemblingStarted;
    };

    void PrintPESH() const { m_PESH.Print(); }
    uint8_t *getBuffer() const { return m_Buffer; }
    int32_t getNumPacketBytes() const { return m_BufferSize; }
    FILE *getOfs() const { return ofs; }

protected:
    void xBufferReset() {
        m_LastContinuityCounter = 0;
        m_BufferSize = 0;
        m_DataOffset = 0;
        delete [] m_Buffer;
        m_Buffer = nullptr;
        m_TmpBuffer = nullptr;
        m_pesOffset = 0;
        m_DataOffset = 0;

        m_PESH.Reset();
    };

    void xBufferAppend(const uint8_t *Data, int32_t Size) {
        if (m_Buffer == nullptr) {
            if(m_PID == 174) m_BufferSize += xTS::TS_PacketLength - Size;
            m_DataOffset += xTS::TS_PacketLength - Size;
            m_Buffer = new uint8_t[m_BufferSize];
            copy(Data + Size, Data + xTS::TS_PacketLength, m_Buffer);

        } else {
            if (m_PID == 136) {
                copy(Data + Size, Data + xTS::TS_PacketLength, m_Buffer + m_DataOffset);
                m_DataOffset += xTS::TS_PacketLength - Size;

            } else if (m_PID == 174) {
                m_BufferSize += (xTS::TS_PacketLength - Size);
                m_TmpBuffer = new uint8_t[m_BufferSize];
                move(m_Buffer + 0, m_Buffer + m_DataOffset, m_TmpBuffer);

                delete[] m_Buffer;
                m_Buffer = m_TmpBuffer;

                copy(Data + Size, Data + xTS::TS_PacketLength, m_Buffer + m_DataOffset);
                m_DataOffset += xTS::TS_PacketLength - Size;
            }
        }
    }
};

int main( int argc, char *argv[ ], char *envp[ ]) {
    FILE *file = fopen("/home/loobson/Politechnika/rewrite/example_new.ts", "rb");

    if (file == nullptr) {
        printf("wrong file name\n");
        return EXIT_FAILURE;
    }

    uint8_t TS_PacketBuffer[xTS::TS_PacketLength];
    xTS_PacketHeader TS_PacketHeader;
    xTS_AdaptationField TS_PacketAdaptationField;
    xPES_Assembler PES_Assembler136;
    xPES_Assembler PES_Assembler174;
    xPES_Assembler::eResult Result;

    int32_t TS_PacketId = 0;

    PES_Assembler136.Init(136);
    PES_Assembler174.Init(174);

    while (!feof(file)) {
        size_t NumRead = fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, file);
        if (NumRead != xTS::TS_PacketLength) { break; }

        TS_PacketHeader.Reset();
        TS_PacketHeader.Parse(TS_PacketBuffer);

        if (TS_PacketHeader.getSyncByte() == 71) {
            printf("%010d ", TS_PacketId);
            TS_PacketHeader.Print();
            TS_PacketAdaptationField.Reset();

            if (TS_PacketHeader.hasAdaptationField()) {
                TS_PacketAdaptationField.Parse(TS_PacketBuffer, TS_PacketHeader.getAdaptationFieldControl());
                TS_PacketAdaptationField.Print();
            }

            if (TS_PacketHeader.getPacketIdentifier() == 136) {
                Result = PES_Assembler136.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_PacketAdaptationField);
            } else if (TS_PacketHeader.getPacketIdentifier() == 174) {
                Result = PES_Assembler174.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_PacketAdaptationField);
            }


            switch (Result) {
                case xPES_Assembler::eResult::StreamPacketLost  :
                    printf(" PES: PcktLost");
                    break;
                case xPES_Assembler::eResult::AssemblingStarted :
                    printf(" PES: Started assembling,");
                    if (TS_PacketHeader.getPacketIdentifier() == 136) PES_Assembler136.PrintPESH();
                    else if (TS_PacketHeader.getPacketIdentifier() == 174) PES_Assembler174.PrintPESH();
                    break;
                case xPES_Assembler::eResult::AssemblingContinue:
                    printf(" PES: Continue");
                    break;
                case xPES_Assembler::eResult::AssemblingFinished:
                    if (TS_PacketHeader.getPacketIdentifier() == 136)
                        printf(" PES: Finished, Len=%4d", PES_Assembler136.getNumPacketBytes());
                    else if (TS_PacketHeader.getPacketIdentifier() == 174)
                        printf(" PES: Finished, Len=%4d", PES_Assembler174.getNumPacketBytes());
                    break;
                default:
                    break;
            }
            printf("\n");
        }
        TS_PacketId++;
    }
    if(PES_Assembler136.getOfs()){
        fwrite(PES_Assembler136.getBuffer(), PES_Assembler136.getNumPacketBytes(), 1, PES_Assembler136.getOfs());
        fclose(PES_Assembler136.getOfs());
    }
    if(PES_Assembler174.getOfs()){
        fwrite(PES_Assembler174.getBuffer(), PES_Assembler174.getNumPacketBytes(), 1, PES_Assembler174.getOfs());
        fclose(PES_Assembler174.getOfs());
    }
    fclose(file);
    return 0;
}