// Specification file for LagLog class

class LAGLOG
{
public:
	LAGLOG();
	void reset();

	void compress_data();
	bool Get_already_compressed();
	void Reset_already_compressed();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	void SetLagInfo(int frame, bool lagFlag);
	void EraseFrame(int frame);
	void InsertFrame(int frame, bool lagFlag, int frames = 1);

	int GetSize();
	bool GetLagInfoAtFrame(int frame);

private:
	// saved data
	std::vector<uint8> lag_log_compressed;

	// not saved data
	std::vector<uint8> lag_log;
	bool already_compressed;			// to compress only once
};
