/*************************************************************
  verify.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <engine.h>
#include <MovesGenerator.h>
#include <Helpers.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace NEngine
{

////////////////////////////////////////////////////////////////////////
void Engine::saveHash(std::string const&  fname) const
{
  if(fname.empty())
    return;
  std::string gfname, cfname, bfname, hfname;
  gfname = fname + "_g.hash";
  cfname = fname + "_c.hash";
  bfname = fname + "_b.hash";
  hfname = fname + "_h.hash";

#ifdef USE_HASH
  hash_.save(gfname);
#endif

  std::ofstream ofs(bfname, std::ios::out);
  save(scontexts_[0].board_, ofs);
  MovesGeneratorBase::save_history(hfname);
}

void Engine::loadHash(std::string const& fname)
{
  std::string gfname, cfname, bfname, hfname;
  gfname = fname + "_g.hash";
  cfname = fname + "_c.hash";
  bfname = fname + "_b.hash";
  hfname = fname + "_h.hash";

#ifdef USE_HASH
  hash_.load(gfname);
#endif

  std::ifstream ifs(bfname, std::ios::in);
  load(scontexts_[0].board_, ifs);
  MovesGenerator::load_history(hfname);
}

////////////////////////////////////////////////////////////////////////////
void Engine::logPV()
{
  if(!callbacks_.slog_)
    return;

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");

  SBoard<Board::GameLength> board(scontexts_[0].board_);

  oss << "iter " << sdata_.depth_ << " ";
  if(sdata_.best_)
  {
    auto strbest = printSAN(board, sdata_.best_);
    if(!strbest.empty())
    {
      oss << " bm " << strbest << " (" << (int)sdata_.best_.vsort_ - (int)ScoreMax << ") ";
    }
  }

  oss << "pv ";
  for(int i = 0; i < MaxPly; ++i)
  {
    Move move = scontexts_[0].plystack_[0].pv_[i];
    if(!move)
      break;
    auto str = printSAN(board, move);
    if(str.empty())
      break;
    board.makeMove(move);
    oss << str << " (" << (int)move.vsort_ - (int)ScoreMax << ") ";
  }
  (*callbacks_.slog_) << oss.str() << std::endl;
}

void Engine::logMovies()
{
  if(!callbacks_.slog_)
    return;

  SBoard<Board::GameLength> board(scontexts_[0].board_);

  auto fen = NEngine::toFEN(board);

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");
  oss << " position " << fen << " ";
  oss << " halfmovies count " << scontexts_[0].board_.halfmovesCount() << " ";
  oss << "iter " << sdata_.depth_ << " ";
  oss << "movies ";

  for(int i = 0; i < sdata_.numOfMoves_; ++i)
  {
    if(checkForStop())
      break;

    Move move = scontexts_[0].moves_[i];
    auto str = printSAN(board, move);
    if(str.empty())
      break;
    oss << str << " {" << (int)move.vsort_ - (int)ScoreMax << "} ";
  }
  (*callbacks_.slog_) << oss.str() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_ESCAPE_GENERATOR
void Engine::verifyEscapeGen(int ictx, const Move & hmove)
{
  if ( !scontexts_[ictx].board_.underCheck() )
    return;

  EscapeGenerator eg(hmove, scontexts_[ictx].board_);
  MovesGenerator mg(scontexts_[ictx].board_);
  X_ASSERT( mg.has_duplicates(), "move generator generates some move twice" );
  X_ASSERT( eg.has_duplicates(), "escape generator generates some move twice" );

  Move legal[Board::MovesMax];
  int num = 0;

  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    Board board0(scontexts_[ictx].board_);

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].figureDir().dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;

      if ( scontexts_[ictx].board_.see(move) < 0 )
        continue;
    }

    legal[num++] = move;

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();

    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );

    scontexts_[ictx].board_.verifyMasks();

    if ( !eg.find(move) )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      EscapeGenerator eg1(hmove, scontexts_[ictx].board_);
      {
        if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
        {
          if ( move.new_type_ != Figure::TypeKnight )
            continue;

          int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
          int dir = scontexts_[ictx].figureDir().dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
          if ( dir < 0 )
            continue;

          if ( scontexts_[ictx].board_.see(move) < 0 )
            continue;
        }
      }

      X_ASSERT( true, "some legal escape from check wasn't generated" );
    }
  }

  X_ASSERT(eg.count() != num, "number of escape moves isn't correct");

  for ( ;; )
  {
    const Move & move = eg.escape();
    if ( !move )
      break;

    Board board0(scontexts_[ictx].board_);

    if ( !scontexts_[ictx].board_.validateMove(move) )
    {
      X_ASSERT( true, "illegal move generated by escape generator" );
    }

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();

    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );

    scontexts_[ictx].board_.verifyMasks();

    bool found = false;
    for (int i = 0; !found && i < num; ++i)
    {
      if ( legal[i] == move )
        found = true;
    }

    X_ASSERT( !found, "move from escape generator isn't found in the legal moves list" );
  }
}
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CHECKS_GENERATOR
void Engine::verifyChecksGenerator(int ictx)
{
  Move killer;
  killer.clear();
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  ChecksGenerator ckg(scontexts_[ictx].board_);
  ckg.generate();

  Move legal[Board::MovesMax], checks[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( move.capture_ || move.new_type_ )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);

    if ( scontexts_[ictx].board_.underCheck() )
      legal[n++] = move;

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );
    scontexts_[ictx].board_.verifyMasks();
  }

  X_ASSERT( mg.count() != num, "not all moves were enumerated" );

  for (int i = 0; i < ckg.count(); ++i)
  {
    const Move & move = ckg[i];
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);

    X_ASSERT( !scontexts_[ictx].board_.underCheck(), "non checking move" );

    checks[m++] = move;

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );
    scontexts_[ictx].board_.verifyMasks();
  }

  for (int i = 0; i < n; ++i)
  {
    const Move & move = legal[i];

    bool found = false;
    for (int j = 0; j < m; ++j)
    {
      const Move & cmove = checks[j];
      if ( cmove == move )
      {
        found = true;
        break;
      }
    }

    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      ChecksGenerator ckg1(scontexts_[ictx].board_);
      ckg1.generate();
    }

    X_ASSERT( !found, "some check wasn't generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & cmove = checks[i];

    bool found = false;
    for (int j = 0; j < n; ++j)
    {
      const Move & move = legal[j];
      if ( move == cmove )
      {
        found = true;
        break;
      }
    }

    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      ChecksGenerator ckg1(scontexts_[ictx].board_);
      ckg1.generate();
    }

    X_ASSERT( !found, "some invalid check was generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & move = checks[i];
    for (int j = i+1; j < m; ++j)
    {
      if ( move == checks[j] )
      {
        char fen[256];
        scontexts_[ictx].board_.toFEN(fen);
        ChecksGenerator ckg1(scontexts_[ictx].board_);
        ckg1.generate();
        X_ASSERT( true, "duplicate move generated by checks generator" );
      }
    }
  }
}
#endif
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CAPS_GENERATOR
void Engine::verifyCapsGenerator(int ictx)
{
  Move hmove, killer;
  hmove.clear();
  killer.clear();
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  CapsGenerator cg(hmove, scontexts_[ictx].board_);

  Figure::Color ocolor = Figure::otherColor(scontexts_[ictx].board_.getColor());

  Move legal[Board::MovesMax], caps[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

    if ( move.new_type_ == Figure::TypeRook || move.new_type_ == Figure::TypeBishop )
      continue;

    if ( move.capture_ && (move.new_type_ == Figure::TypeQueen || !move.new_type_) )
    {
      legal[n++] = move;
      continue;
    }

    if ( move.new_type_ == Figure::TypeQueen )
    {
      legal[n++] = move;
      continue;
    }
    
    if ( move.new_type_ == Figure::TypeKnight )
    {
      if ( scontexts_[ictx].movesTable().caps(Figure::TypeKnight, move.to_) & scontexts_[ictx].board_.fmgr().king_mask(ocolor) )
        legal[n++] = move;
    }
  }

  X_ASSERT( num != mg.count(), "not all moves were enumerated" );

  for (int i = 0; i < cg.count(); ++i)
  {
    const Move & cap = cg[i];
    if ( !cap )
      break;

    if ( !scontexts_[ictx].board_.validateMove(cap) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(cap);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

    caps[m++] = cap;
  }

  for (int i = 0; i < n; ++i)
  {
    const Move & move = legal[i];

    bool found = false;
    for (int j = 0; j < m; ++j)
    {
      const Move & cap = caps[j];
      if ( cap == move )
      {
        found = true;
        break;
      }
    }

    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      CapsGenerator cg1(hmove, scontexts_[ictx].board_);
    }

    X_ASSERT( !found, "some capture wasn't generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & cap = caps[i];

    bool found = false;
    for (int j = 0; j < n; ++j)
    {
      const Move & move = legal[j];
      if ( move == cap )
      {
        found = true;
        break;
      }
    }
    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      CapsGenerator cg1(hmove, scontexts_[ictx].board_);
    }

    X_ASSERT( !found, "some invalid capture was generated" );
  }
}
#endif


#ifdef VERIFY_FAST_GENERATOR
void Engine::verifyFastGenerator(int ictx, const Move & hmove, const Move & killer)
{
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  FastGenerator fg(scontexts_[ictx].board_, hmove, killer);

  Move legal[Board::MovesMax];
  Move fast[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);
    scontexts_[ictx].board_.makeMove(move);

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    X_ASSERT( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].figureDir().dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;
    }

    legal[n++] = move;
  }

  for ( ;; )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    fast[m++] = move;
  }


  for (int i = 0; i < n; ++i)
  {
    Move & move = legal[i];
    bool found = false;
    for (int j = 0; !found && j < m; ++j)
    {
      if ( move == fast[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
      while ( fg1.move() );
    }

    X_ASSERT( !found, "move wasn't generated by fast generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    Move & move = fast[i];
    bool found = false;
    for (int j = 0; !found && j < n; ++j)
    {
      if ( move == legal[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
      while ( fg1.move() );
    }

    X_ASSERT( !found, "odd move was generated by fast generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    for (int j = i+1; j < m; ++j)
    {
      if ( fast[i] == fast[j] )
      {
        char str[256];
        scontexts_[ictx].board_.toFEN(str);
        FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
        while ( fg1.move() );
      }
      X_ASSERT( fast[i] == fast[j], "duplicate found in fast generator" );
    }
  }
}
#endif


#ifdef VERIFY_TACTICAL_GENERATOR
void Engine::verifyTacticalGenerator(int ictx)
{
  Figure::Type minimalType = Figure::TypePawn;
  MovesGenerator mg(scontexts_[ictx].board_);
  TacticalGenerator tg(scontexts_[ictx].board_, minimalType, 0);

  Move legal[Board::MovesMax], tactical[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( scontexts_[ictx].board_.underCheck() )
    {
      legal[n++] = move;
      continue;
    }

    scontexts_[ictx].board_.makeMove(move);
    bool check = scontexts_[ictx].board_.underCheck();
    scontexts_[ictx].board_.unmakeMove();

    if ( !check && !move.capture_ && !move.new_type_ )
      continue;

    if ( move.new_type_ == Figure::TypeBishop || move.new_type_ == Figure::TypeRook )
      continue;

    int ss = scontexts_[ictx].board_.see(move);
    if ( ss < 0 )
      continue;

    if ( !check && move.new_type_ == Figure::TypeKnight )
      continue;

    if ( move.new_type_ == Figure::TypeKnight )
    {
      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].figureDir().dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;
    }

    legal[n++] = move;
  }

  for ( ;; )
  {
    Move & move = tg.next();
    if ( !move )
      break;

    if ( scontexts_[ictx].board_.validateMove(move) )
      tactical[m++] = move;
  }

  for (int i = 0; i < n; ++i)
  {
    Move & move = legal[i];
    bool found = false;
    for (int j = 0; !found && j < m; ++j)
    {
      if ( move == tactical[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
      while ( tg1.next() );
    }

    X_ASSERT( !found, "move wasn't generated by tactical generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    Move & move = tactical[i];
    bool found = false;
    for (int j = 0; !found && j < n; ++j)
    {
      if ( move == legal[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
      while ( tg1.next() );
    }

    X_ASSERT( !found, "odd move was generated by tactical generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    for (int j = i+1; j < m; ++j)
    {
      if ( tactical[i] == tactical[j] )
      {
        char str[256];
        scontexts_[ictx].board_.toFEN(str);
        TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
        while ( tg1.next() );
      }
      X_ASSERT( tactical[i] == tactical[j], "duplicate found in tactical generator" );
    }
  }
}
#endif

void Engine::verifyGenerators(int /*ictx*/, const Move & /*hmove*/)
{

#ifdef VERIFY_ESCAPE_GENERATOR
  verifyEscapeGen(ictx, hmove);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  verifyChecksGenerator(ictx);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  verifyCapsGenerator(ictx);
#endif

#ifdef VERIFY_FAST_GENERATOR
  Move killer(0);
  verifyFastGenerator(ictx, hmove, killer);
#endif

#ifdef VERIFY_TACTICAL_GENERATOR
  verifyTacticalGenerator(ictx);
#endif

}

void Engine::enumerate()
{
  enumerate(sdata_.depth_);
}

void Engine::enumerate(int /*depth*/)
{
}

//////////////////////////////////////////////////////////////////////////
/// for DEBUG
void Engine::findSequence(int ictx, const Move & move, int ply, int depth,
  int counter, ScoreType alpha, ScoreType betta) const
{
  struct MOVE { int from_, to_; };
  bool identical = false;
  static MOVE sequence[] = {
    {52, 45},
    {9, 8},
     };

    if ( ply < sizeof(sequence)/sizeof(MOVE) && move.from_ == sequence[ply].from_ && move.to_ == sequence[ply].to_ )
    {
      for (int i = ply; i >= 0; --i)
      {
        identical = true;
        int j = i-ply;
        if ( j >= scontexts_[ictx].board_.halfmovesCount() )
          break;
        const UndoInfo & undo = scontexts_[ictx].board_.undoInfoRev(j);
        if ( undo.from_ != sequence[i].from_ || undo.to_ != sequence[i].to_ )
        {
          identical = false;
          break;
        }
      }
    }

    if ( identical )
    {
      //if ( sdata_.depth_ == 5*ONE_PLY && ply == 1 )
      //{
      //  int ttt = 0;
      //}
      std::ostringstream oss;
      NEngine::save(scontexts_[ictx].board_, oss, false);
      oss << "PLY: " << ply << std::endl;
      oss << "depth_ = " << sdata_.depth_ << "; depth = " << depth << "; ply = "
        << ply << "; alpha = " << alpha << "; betta = " << betta << "; counter = " << counter << std::endl;
      oss << "===================================================================" << std::endl << std::endl;

      std::ofstream ofs("sequence.txt", std::ios_base::app);
      ofs << oss.str();
    }
}

} // NEngine
