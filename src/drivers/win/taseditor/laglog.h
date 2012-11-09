// Specification file for LagLog class

#define LAGGED_NO 0
#define LAGGED_YES 1
#define LAGGED_DONTKNOW 2

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

	void InvalidateFrom(int frame);

	void SetLagInfo(int frame, bool lagFlag);
	void EraseFrame(int frame, int frames = 1);
	void InsertFrame(int frame, bool lagFlag, int frames = 1);

	int GetSize();
	int GetLagInfoAtFrame(int frame);

	int findFirstChange(LAGLOG& their_log);

private:
	// saved data
	std::vector<uint8> lag_log_compressed;

	// not saved data
	std::vector<uint8> lag_log;
	bool already_compressed;			// to compress only once
};
