namespace litest {

typedef unsigned char uint8_t;
constexpr uint8_t ID_SIZE = 16;

class Identity {
 public:
  explicit Identity(bool need_generate = true);
  //Identity(const Identity& rhs);
  virtual ~Identity();

  bool operator == (const Identity& refIdentity) const;

  void dumpUUid();
 //private:
 // void Update();

 private:
  char data_[ID_SIZE];
};

}  // namespace litest
