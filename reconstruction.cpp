#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Order {
    char side;
    double price;
    long long size;
};

struct Level {
    long long size = 0;
    int count = 0;
};

class Book {
public:
    void add(long long id, char side, double price, long long size) {
        orders[id] = {side, price, size};
        auto &lvl = (side == 'B' ? bids[price] : asks[price]);
        lvl.size += size;
        lvl.count += 1;
    }

    void cancel(long long id, long long size) {
        auto it = orders.find(id);
        if (it == orders.end()) return;
        Order &o = it->second;
        long long amt = std::min(size, o.size);
        o.size -= amt;
        auto &lvl = (o.side == 'B' ? bids[o.price] : asks[o.price]);
        if (lvl.size >= amt)
            lvl.size -= amt;
        else
            lvl.size = 0;
        if (o.size == 0) {
            lvl.count -= 1;
            orders.erase(it);
        }
        if (lvl.size == 0 && lvl.count == 0) {
            if (o.side == 'B')
                bids.erase(o.price);
            else
                asks.erase(o.price);
        }
    }

    void clear() {
        orders.clear();
        bids.clear();
        asks.clear();
    }

    std::string snapshot() const {
        std::ostringstream out;
        int depth = 0;
        for (auto it = bids.begin(); it != bids.end() && depth < 10; ++it, ++depth)
            out << ',' << std::fixed << std::setprecision(2) << it->first << ',' << it->second.size << ',' << it->second.count;
        for (; depth < 10; ++depth) out << ',' << ',' << 0 << ',' << 0;
        depth = 0;
        for (auto it = asks.begin(); it != asks.end() && depth < 10; ++it, ++depth)
            out << ',' << std::fixed << std::setprecision(2) << it->first << ',' << it->second.size << ',' << it->second.count;
        for (; depth < 10; ++depth) out << ',' << ',' << 0 << ',' << 0;
        return out.str();
    }

private:
    std::unordered_map<long long, Order> orders;
    std::map<double, Level, std::greater<double>> bids;
    std::map<double, Level> asks;
};

static std::vector<std::string> split_csv(const std::string &line) {
    std::vector<std::string> res;
    std::string cell;
    std::istringstream ss(line);
    while (std::getline(ss, cell, ',')) res.push_back(cell);
    return res;
}

struct PendingTrade {
    std::vector<std::string> fields;
    long long order_id = 0;
    char book_side = ' ';
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mbo.csv>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Failed to open input file" << std::endl;
        return 1;
    }

    std::string header;
    std::getline(in, header); // consume MBO header, not used

    Book book;

    std::cout
        << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence"
        << ",bid_px_00,bid_sz_00,bid_ct_00,ask_px_00,ask_sz_00,ask_ct_00"
        << ",bid_px_01,bid_sz_01,bid_ct_01,ask_px_01,ask_sz_01,ask_ct_01"
        << ",bid_px_02,bid_sz_02,bid_ct_02,ask_px_02,ask_sz_02,ask_ct_02"
        << ",bid_px_03,bid_sz_03,bid_ct_03,ask_px_03,ask_sz_03,ask_ct_03"
        << ",bid_px_04,bid_sz_04,bid_ct_04,ask_px_04,ask_sz_04,ask_ct_04"
        << ",bid_px_05,bid_sz_05,bid_ct_05,ask_px_05,ask_sz_05,ask_ct_05"
        << ",bid_px_06,bid_sz_06,bid_ct_06,ask_px_06,ask_sz_06,ask_ct_06"
        << ",bid_px_07,bid_sz_07,bid_ct_07,ask_px_07,ask_sz_07,ask_ct_07"
        << ",bid_px_08,bid_sz_08,bid_ct_08,ask_px_08,ask_sz_08,ask_ct_08"
        << ",bid_px_09,bid_sz_09,bid_ct_09,ask_px_09,ask_sz_09,ask_ct_09"
        << ",symbol,order_id"
        << std::endl;

    size_t index = 0;
    PendingTrade pending;
    bool have_pending = false;

    std::string line;
    while (std::getline(in, line)) {
        auto f = split_csv(line);
        if (f.size() < 15) continue; // malformed line
        char action = f[5].empty() ? ' ' : f[5][0];
        char side = f[6].empty() ? ' ' : f[6][0];
        double price = f[7].empty() ? 0.0 : std::stod(f[7]);
        long long size = f[8].empty() ? 0 : std::stoll(f[8]);
        long long order_id = f[10].empty() ? 0 : std::stoll(f[10]);

        auto print_row = [&](char act, char sd, const std::vector<std::string> &base) {
            std::ostringstream out;
            out << index++ << ',' << base[0] << ',' << base[1] << ",10," << base[3] << ',' << base[4] << ','
                << act << ',' << sd << ",0," << base[7] << ',' << base[8] << ','
                << base[11] << ',' << base[12] << ',' << base[13]
                << book.snapshot() << ',' << base[14] << ',' << base[10];
            std::cout << out.str() << std::endl;
        };

        if (have_pending) {
            if (action == 'F') {
                pending.order_id = order_id;
                pending.book_side = side;
                continue; // wait for cancel
            }
            if (action == 'C' && order_id == pending.order_id) {
                book.cancel(order_id, size);
                print_row('T', pending.book_side, pending.fields);
                have_pending = false;
                continue; // skip printing this cancel
            }
            // unexpected: discard pending
            have_pending = false;
        }

        switch (action) {
        case 'A':
            book.add(order_id, side, price, size);
            print_row('A', side, f);
            break;
        case 'C':
            book.cancel(order_id, size);
            print_row('C', side, f);
            break;
        case 'R':
            book.clear();
            print_row('R', side, f);
            break;
        case 'T':
            if (side == 'N') {
                print_row('T', side, f);
            } else {
                pending.fields = f;
                have_pending = true;
            }
            break;
        default:
            // ignore unknown actions
            break;
        }
    }

    return 0;
}
