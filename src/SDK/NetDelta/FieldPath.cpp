#include "FieldPath.h"
#include <queue>

namespace NetDelta
{
    namespace
    {
        constexpr int OpCount = 40;
        constexpr int OpFinish = 39;

        constexpr int OpWeights[OpCount] = {
            36271, 10334, 1375, 646, 4128,
            35, 3, 521, 2942, 560, 471, 10530, 251,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 310,
            2, 1, 1837, 149, 300, 634, 1, 1, 1,
            76, 271, 99,
            25474
        };

        struct HuffNode
        {
            int Weight;
            int Value;
            int Left = -1;
            int Right = -1;
        };

        struct Tree
        {
            std::vector<HuffNode> Nodes;
            int Root = -1;
            std::vector<uint32_t> Codes;
            std::vector<uint32_t> CodeLens;
        };

        void BuildCodes(Tree& T, int Node, uint32_t Code, uint32_t Len)
        {
            const HuffNode& N = T.Nodes[Node];
            if (N.Left < 0)
            {
                T.Codes[N.Value]    = Code;
                T.CodeLens[N.Value] = Len;
                return;
            }
            BuildCodes(T, N.Left, Code, Len + 1);
            BuildCodes(T, N.Right, Code | (1u << Len), Len + 1);
        }

        const Tree& GetTree()
        {
            static const Tree Built = []
            {
                Tree T;
                auto Less = [&T](int A, int B)
                {
                    const HuffNode& X = T.Nodes[A];
                    const HuffNode& Y = T.Nodes[B];
                    if (X.Weight == Y.Weight)
                        return X.Value >= Y.Value;
                    return X.Weight < Y.Weight;
                };
                auto Cmp = [&Less](int A, int B) { return Less(B, A); };
                std::priority_queue<int, std::vector<int>, decltype(Cmp)> Heap(Cmp);

                T.Nodes.reserve(OpCount * 2);
                for (int I = 0; I < OpCount; ++I)
                {
                    T.Nodes.push_back({ OpWeights[I] ? OpWeights[I] : 1, I });
                    Heap.push(I);
                }

                int NextValue = OpCount;
                while (Heap.size() > 1)
                {
                    int A = Heap.top(); Heap.pop();
                    int B = Heap.top(); Heap.pop();
                    HuffNode Parent{ T.Nodes[A].Weight + T.Nodes[B].Weight, NextValue++, A, B };
                    T.Nodes.push_back(Parent);
                    Heap.push(static_cast<int>(T.Nodes.size() - 1));
                }
                T.Root = Heap.top();
                T.Codes.assign(OpCount, 0);
                T.CodeLens.assign(OpCount, 0);
                BuildCodes(T, T.Root, 0, 0);
                return T;
            }();
            return Built;
        }

        void Pop(FieldPath& Fp, int Count)
        {
            for (int I = 0; I < Count && Fp.Last > 0; ++I)
            {
                Fp.Path[Fp.Last] = 0;
                --Fp.Last;
            }
        }

        bool Push(FieldPath& Fp, int Value)
        {
            if (Fp.Last >= 6)
                return false;
            ++Fp.Last;
            Fp.Path[Fp.Last] = Value;
            return true;
        }

        bool ApplyOp(int Op, BitReader& R, FieldPath& Fp)
        {
            switch (Op)
            {
            case 0: Fp.Path[Fp.Last] += 1; return true;
            case 1: Fp.Path[Fp.Last] += 2; return true;
            case 2: Fp.Path[Fp.Last] += 3; return true;
            case 3: Fp.Path[Fp.Last] += 4; return true;
            case 4: Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVarFP()) + 5; return true;
            case 5: return Push(Fp, 0);
            case 6: return Push(Fp, static_cast<int>(R.ReadUBitVarFP()));
            case 7: Fp.Path[Fp.Last] += 1; return Push(Fp, 0);
            case 8: Fp.Path[Fp.Last] += 1; return Push(Fp, static_cast<int>(R.ReadUBitVarFP()));
            case 9: Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVarFP()); return Push(Fp, 0);
            case 10:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVarFP()) + 2;
                return Push(Fp, static_cast<int>(R.ReadUBitVarFP()) + 1);
            }
            case 11:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadBits(3)) + 2;
                return Push(Fp, static_cast<int>(R.ReadBits(3)) + 1);
            }
            case 12:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadBits(4)) + 2;
                return Push(Fp, static_cast<int>(R.ReadBits(4)) + 1);
            }
            case 13:
            {
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B);
            }
            case 14:
            {
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B);
            }
            case 15:
            {
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                int C = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 16:
            {
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                int C = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 17:
            {
                Fp.Path[Fp.Last] += 1;
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B);
            }
            case 18:
            {
                Fp.Path[Fp.Last] += 1;
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B);
            }
            case 19:
            {
                Fp.Path[Fp.Last] += 1;
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                int C = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 20:
            {
                Fp.Path[Fp.Last] += 1;
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                int C = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 21:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVar()) + 2;
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B);
            }
            case 22:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVar()) + 2;
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B);
            }
            case 23:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVar()) + 2;
                int A = static_cast<int>(R.ReadUBitVarFP());
                int B = static_cast<int>(R.ReadUBitVarFP());
                int C = static_cast<int>(R.ReadUBitVarFP());
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 24:
            {
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVar()) + 2;
                int A = static_cast<int>(R.ReadBits(5));
                int B = static_cast<int>(R.ReadBits(5));
                int C = static_cast<int>(R.ReadBits(5));
                return Push(Fp, A) && Push(Fp, B) && Push(Fp, C);
            }
            case 25:
            {
                int N = static_cast<int>(R.ReadUBitVar());
                Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVar());
                for (int I = 0; I < N; ++I)
                    if (!Push(Fp, static_cast<int>(R.ReadUBitVarFP())))
                        return false;
                return true;
            }
            case 26:
            {
                for (int I = 0; I <= Fp.Last; ++I)
                    if (R.ReadBool())
                        Fp.Path[I] += R.ReadVarInt32() + 1;
                int Count = static_cast<int>(R.ReadUBitVar());
                for (int I = 0; I < Count; ++I)
                    if (!Push(Fp, static_cast<int>(R.ReadUBitVarFP())))
                        return false;
                return true;
            }
            case 27: Pop(Fp, 1); Fp.Path[Fp.Last] += 1; return true;
            case 28: Pop(Fp, 1); Fp.Path[Fp.Last] += static_cast<int>(R.ReadUBitVarFP()) + 1; return true;
            case 29: Pop(Fp, Fp.Last); Fp.Path[0] += 1; return true;
            case 30: Pop(Fp, Fp.Last); Fp.Path[0] += static_cast<int>(R.ReadUBitVarFP()) + 1; return true;
            case 31: Pop(Fp, Fp.Last); Fp.Path[0] += static_cast<int>(R.ReadBits(3)) + 1; return true;
            case 32: Pop(Fp, Fp.Last); Fp.Path[0] += static_cast<int>(R.ReadBits(6)) + 1; return true;
            case 33: Pop(Fp, static_cast<int>(R.ReadUBitVarFP())); Fp.Path[Fp.Last] += 1; return true;
            case 34: Pop(Fp, static_cast<int>(R.ReadUBitVarFP())); Fp.Path[Fp.Last] += R.ReadVarInt32(); return true;
            case 35:
            {
                Pop(Fp, static_cast<int>(R.ReadUBitVarFP()));
                for (int I = 0; I <= Fp.Last; ++I)
                    if (R.ReadBool())
                        Fp.Path[I] += R.ReadVarInt32();
                return true;
            }
            case 36:
            {
                for (int I = 0; I <= Fp.Last; ++I)
                    if (R.ReadBool())
                        Fp.Path[I] += R.ReadVarInt32();
                return true;
            }
            case 37:
                if (Fp.Last < 1)
                    return false;
                Fp.Path[Fp.Last - 1] += 1;
                return true;
            case 38:
            {
                for (int I = 0; I <= Fp.Last; ++I)
                    if (R.ReadBool())
                        Fp.Path[I] += static_cast<int>(R.ReadBits(4)) - 7;
                return true;
            }
            default:
                return false;
            }
        }

        void WriteOp(BitWriter& W, int Op)
        {
            const Tree& T = GetTree();
            W.WriteBits(T.Codes[Op], T.CodeLens[Op]);
        }

        bool PrefixEqual(const FieldPath& A, const FieldPath& B, int Count)
        {
            for (int I = 0; I < Count; ++I)
                if (A.Path[I] != B.Path[I])
                    return false;
            return true;
        }

        bool EncodeTransition(BitWriter& W, const FieldPath& P, const FieldPath& N)
        {
            int Pl = P.Last;
            int Nl = N.Last;

            if (Nl == Pl && PrefixEqual(P, N, Nl) && N.Path[Nl] > P.Path[Nl])
            {
                int D = N.Path[Nl] - P.Path[Nl];
                if (D <= 4)
                {
                    WriteOp(W, D - 1);
                }
                else
                {
                    WriteOp(W, 4);
                    W.WriteUBitVarFP(static_cast<uint32_t>(D - 5));
                }
                return true;
            }

            if (Nl == Pl + 1 && PrefixEqual(P, N, Pl) && N.Path[Pl] >= P.Path[Pl] && N.Path[Nl] >= 0)
            {
                int D = N.Path[Pl] - P.Path[Pl];
                int V = N.Path[Nl];
                if (D == 0)
                {
                    if (V == 0)
                        WriteOp(W, 5);
                    else
                    {
                        WriteOp(W, 6);
                        W.WriteUBitVarFP(static_cast<uint32_t>(V));
                    }
                }
                else if (D == 1)
                {
                    if (V == 0)
                        WriteOp(W, 7);
                    else
                    {
                        WriteOp(W, 8);
                        W.WriteUBitVarFP(static_cast<uint32_t>(V));
                    }
                }
                else if (V >= 1)
                {
                    if (D - 2 < 8 && V - 1 < 8)
                    {
                        WriteOp(W, 11);
                        W.WriteBits(static_cast<uint32_t>(D - 2), 3);
                        W.WriteBits(static_cast<uint32_t>(V - 1), 3);
                    }
                    else
                    {
                        WriteOp(W, 10);
                        W.WriteUBitVarFP(static_cast<uint32_t>(D - 2));
                        W.WriteUBitVarFP(static_cast<uint32_t>(V - 1));
                    }
                }
                else
                {
                    WriteOp(W, 9);
                    W.WriteUBitVarFP(static_cast<uint32_t>(D));
                }
                return true;
            }

            if (Nl < Pl)
            {
                if (Nl == 0 && N.Path[0] > P.Path[0])
                {
                    int D = N.Path[0] - P.Path[0];
                    if (D == 1)
                    {
                        WriteOp(W, 29);
                    }
                    else if (D - 1 < 8)
                    {
                        WriteOp(W, 31);
                        W.WriteBits(static_cast<uint32_t>(D - 1), 3);
                    }
                    else if (D - 1 < 64)
                    {
                        WriteOp(W, 32);
                        W.WriteBits(static_cast<uint32_t>(D - 1), 6);
                    }
                    else
                    {
                        WriteOp(W, 30);
                        W.WriteUBitVarFP(static_cast<uint32_t>(D - 1));
                    }
                    return true;
                }

                if (Nl == Pl - 1 && PrefixEqual(P, N, Nl) && N.Path[Nl] > P.Path[Nl])
                {
                    int D = N.Path[Nl] - P.Path[Nl];
                    if (D == 1)
                    {
                        WriteOp(W, 27);
                    }
                    else
                    {
                        WriteOp(W, 28);
                        W.WriteUBitVarFP(static_cast<uint32_t>(D - 1));
                    }
                    return true;
                }

                WriteOp(W, 35);
                W.WriteUBitVarFP(static_cast<uint32_t>(Pl - Nl));
                for (int I = 0; I <= Nl; ++I)
                {
                    int D = N.Path[I] - P.Path[I];
                    W.WriteBool(D != 0);
                    if (D != 0)
                        W.WriteVarInt32(D);
                }
                return true;
            }

            if (Nl == Pl)
            {
                WriteOp(W, 36);
                for (int I = 0; I <= Nl; ++I)
                {
                    int D = N.Path[I] - P.Path[I];
                    W.WriteBool(D != 0);
                    if (D != 0)
                        W.WriteVarInt32(D);
                }
                return true;
            }

            WriteOp(W, 26);
            for (int I = 0; I <= Pl; ++I)
            {
                int D = N.Path[I] - P.Path[I];
                W.WriteBool(D != 0);
                if (D != 0)
                    W.WriteVarInt32(D - 1);
            }
            W.WriteUBitVar(static_cast<uint32_t>(Nl - Pl));
            for (int I = Pl + 1; I <= Nl; ++I)
            {
                if (N.Path[I] < 0)
                    return false;
                W.WriteUBitVarFP(static_cast<uint32_t>(N.Path[I]));
            }
            return true;
        }
    }

    bool FieldPath::operator==(const FieldPath& Other) const
    {
        if (Last != Other.Last)
            return false;
        for (int I = 0; I <= Last; ++I)
            if (Path[I] != Other.Path[I])
                return false;
        return true;
    }

    bool FieldPath::operator<(const FieldPath& Other) const
    {
        int Common = Last < Other.Last ? Last : Other.Last;
        for (int I = 0; I <= Common; ++I)
        {
            if (Path[I] != Other.Path[I])
                return Path[I] < Other.Path[I];
        }
        return Last < Other.Last;
    }

    std::string FieldPath::Key() const
    {
        std::string Out;
        Out.reserve((Last + 1) * 2);
        for (int I = 0; I <= Last; ++I)
        {
            Out.push_back(static_cast<char>(Path[I] & 0xFF));
            Out.push_back(static_cast<char>((Path[I] >> 8) & 0xFF));
        }
        return Out;
    }

    bool DecodeFieldPaths(BitReader& Reader, std::vector<FieldPath>& Out)
    {
        const Tree& T = GetTree();
        FieldPath Fp;
        int Node = T.Root;
        while (!Reader.Overflow())
        {
            int Next = Reader.ReadBits(1) ? T.Nodes[Node].Right : T.Nodes[Node].Left;
            if (T.Nodes[Next].Left >= 0)
            {
                Node = Next;
                continue;
            }

            Node = T.Root;
            int Op = T.Nodes[Next].Value;
            if (Op == OpFinish)
                return true;
            if (!ApplyOp(Op, Reader, Fp))
                return false;
            Out.push_back(Fp);
        }
        return false;
    }

    bool EncodeFieldPaths(BitWriter& Writer, const std::vector<FieldPath>& Paths)
    {
        FieldPath Prev;
        for (const FieldPath& Next : Paths)
        {
            if (!EncodeTransition(Writer, Prev, Next))
                return false;
            Prev = Next;
        }
        WriteOp(Writer, OpFinish);
        return true;
    }
}
