#include <Board.h>

namespace NEngine
{

extern int64 x_movesCounter;

struct Move2
{
  int8    from;
  int8    to;
  int8    new_type;
  uint8   mask;
  uint32  sort_value;

  inline bool operator < (Move2 const& m) const
  {
    return sort_value < m.sort_value;
  }
};

struct Board2
{
  enum State
  {
    Invalid = 0,
    Ok = 1,
    UnderCheck = 2,
    Stalemat = 4,
    DrawReps = 8,
    DrawInsuf = 16,
    Draw50Moves = 64,
    ChessMat = 128
  };

  enum { NumOfFields = 64, MovesMax = 256, FENsize = 8192, GameLength = 4096 };

  /// is move meet rules
  bool validateMove(const Move2 & mv) const;

  /// make move
  void makeMove(const Move2 &);
  void unmakeMove();

  FiguresManager const& fmgr() const { return fmgr_; }

  /// always use this method to get field
  inline const Field & getField(int index) const
  {
    X_ASSERT((unsigned)index > 63, "field index is invalid");
    return fields_[index];
  }

  /// always use this method to get field
  inline Field & getField(int index)
  {
    X_ASSERT((unsigned)index > 63, "field index is invalid");
    return fields_[index];
  }

  /// add new figure
  bool addFigure(const Figure::Color color, const Figure::Type type, int pos);

  /// position, where capturing pawn goes to
  inline int enpassant() const
  {
    return en_passant_;
  }

  UndoInfo & undoInfo(int i)
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i < 0 || i >= GameLength, "there was no move");
    return g_undoStack[i];
  }

  bool underCheck() const { return (state_ & State::UnderCheck) != 0; }

  /// just a useful method to quickly check if there is a draw
  static bool isDraw(uint8 state)
  {
    return (state & (State::Stalemat | State::DrawReps | State::DrawInsuf | State::Draw50Moves)) != State::Invalid;
  }

  inline bool matState() const { return (state_ & State::ChessMat) != 0; }
  inline bool drawState() const { return isDraw(state_); }

  int checkingNum() const { return checkingNum_; }

  /// returns current move color
  Figure::Color color() const { return color_; }
 
  // initialization methods
  void setColor(Figure::Color c) { color_ = c;  }

  void hashColor() { fmgr_.hashColor(); }

  bool castling(Figure::Color c, int t /* 0 - short (K), 1 - long (Q) */) const
  {
    int offset = ((c<<1) | t);
    return (castling_ >> offset) & 1;
  }

  bool castling(Figure::Color c) const
  {
    int offset = c<<1;
    return ((castling_ >> offset) & 3) != 0;
  }

  /// set short/long castle
  void set_castling(Figure::Color c, int t)
  {
    int offset = ((c<<1) | t);
    castling_ |= set_bit(offset);
    fmgr_.hashCastling(c, t);
  }

  void setEnpassant(int ep, Figure::Color c)
  {
    en_passant_ = ep;
    fmgr_.hashEnPassant(ep, c);
  }

  void setFiftyMovesCount(int c)
  {
    fiftyMovesCount_ = c;
  }

  void setMovesCounter(int c)
  {
    movesCounter_ = c;
  }

  bool invalidate();
  void verifyState();
  bool verifyChessDraw();
  int findCheckingFigures(Figure::Color ocolor, int ki_pos);

private:
  bool fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const;
  
  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int8 pos) const
  {
    BitMask mask_all_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
    return fieldAttacked(c, pos, mask_all_inv);
  }

  inline void setNoMoves()
  {
    if((State::Invalid == state_) || (State::ChessMat & state_) || drawState())
      return;
    if(underCheck())
      state_ |= State::ChessMat;
    else
      state_ |= State::Stalemat;
  }

// data members
private:
  // check
  uint8 checkingNum_{};
  union
  {
    uint8  checking_[2];
    uint16 checking_figs_{};
  };

  uint8 fiftyMovesCount_{};
  int32 halfmovesCounter_{};
  int32 movesCounter_{};

  // castling possibility flag
  // (0, 1) bits block for black
  // (2, 3) bits block for white
  // lower bit in block is short (K)
  // higher bit in block is long (Q)
  uint8 castling_{};

  int8  en_passant_{};

  uint8 state_{ State::Invalid };

  /// color to make move from this position
  Figure::Color color_{};

  /// fields array 8 x 8
  Field fields_[NumOfFields] = {};

  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

protected:
  UndoInfo* g_undoStack{};
};

void xsearch(Board2& board, int depth);

} // NEngine