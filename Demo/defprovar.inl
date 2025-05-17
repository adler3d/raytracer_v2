#define ADD(TYPE,NAME,VALUE)TYPE NAME;
DEF_PRO_VARIABLE(ADD)
#undef ADD
//===
void DoReset()
{
  t_$ $;

  #ifdef DEF_PRO_CLASSPARENT
    #ifndef DEF_PRO_CLASSNAME
      static_assert(false,"DEF_PRO_CLASSNAME not defined but DEF_PRO_CLASSPARENT is defined? WTH!");
    #endif
  (*(DEF_PRO_CLASSPARENT()*)this).DoReset();
  #endif
  #define ADD(TYPE,NAME,VALUE)t_$::set(this->NAME,VALUE);
  DEF_PRO_VARIABLE(ADD)
  #undef ADD
}
#ifdef DEF_PRO_CLASSNAME
  void operator=(DEF_PRO_CLASSNAME()&&ref){oper_set(std::move(ref));}
  DEF_PRO_CLASSNAME()(DEF_PRO_CLASSNAME()&&ref){DoReset();oper_set(std::move(ref));}
  void oper_set(DEF_PRO_CLASSNAME()&&ref)
  {
    #ifdef DEF_PRO_CLASSPARENT
    (*(DEF_PRO_CLASSPARENT()*)this)=std::move((DEF_PRO_CLASSPARENT()&)ref);
    #endif
    #define F(TYPE,NAME,VALUE)this->NAME=std::move(ref.NAME);
    DEF_PRO_VARIABLE(F);
    #undef F
  }
  void operator=(const DEF_PRO_CLASSNAME()&ref){oper_set(ref);}
  DEF_PRO_CLASSNAME()(const DEF_PRO_CLASSNAME()&ref){DoReset();oper_set(ref);}
  void oper_set(const DEF_PRO_CLASSNAME()&ref)
  {
    #ifdef DEF_PRO_CLASSPARENT
    (*(DEF_PRO_CLASSPARENT()*)this)=(DEF_PRO_CLASSPARENT()&)ref;
    #endif
    #define F(TYPE,NAME,VALUE)this->NAME=ref.NAME;
    DEF_PRO_VARIABLE(F);
    #undef F
  }
  DEF_PRO_CLASSNAME()(){DoReset();}
  #undef DEF_PRO_CLASSNAME
#endif //DEF_PRO_CLASSNAME
  
#ifdef DEF_PRO_CLASSPARENT
  #undef DEF_PRO_CLASSPARENT
#endif //DEF_PRO_CLASSPARENT

#ifdef DEF_PRO_UNSAVEABLE
  #undef DEF_PRO_UNSAVEABLE
#endif

#undef DEF_PRO_VARIABLE