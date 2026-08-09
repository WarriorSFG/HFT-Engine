#include<unordered_map>
#include<iostream>
#include<chrono>
#include<cstdint>
#include<map>

using namespace std;

enum OrderType{
    BUY,
    SELL
};

class OrderNode{
    uint64_t orderId;
    OrderType orderType;
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp;

    OrderNode* next;
    OrderNode* prev;

public:
    OrderNode() : orderId(0), prev(nullptr), next(nullptr) {};
    OrderNode(uint64_t id, OrderType type, uint32_t p, uint32_t q)
        : orderId(id), orderType(type), price(p), quantity(q), timestamp(chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count()), next(nullptr), prev(nullptr) {}

    uint64_t getOrderId() const { return orderId; }
    OrderType getOrderType() const { return orderType; }
    uint32_t getPrice() const { return price; }
    uint32_t getQuantity() const { return quantity; }
    uint64_t getTimestamp() const { return timestamp; }
    void reduceQuantity(uint32_t q) { quantity -= q; }
    
    void setNext(OrderNode* n) { next = n; }
    void setPrev(OrderNode* p) { prev = p; }

    OrderNode* getNext() const { return next; }
    OrderNode* getPrev() const { return prev; }
};

class PriceLevel {
    uint32_t limitPrice;
    OrderNode* head;
    OrderNode* tail;
    uint32_t totalVolume;

public:
    PriceLevel(uint32_t price) : limitPrice(price), totalVolume(0), head(nullptr), tail(nullptr) {}

    uint32_t getLimitPrice() const { return limitPrice; }
    uint32_t getTotalVolume() const { return totalVolume; }
    OrderNode* getHead() const { return head; }

    void AddOrder(OrderNode* order) {
        if (head == nullptr) {
            head = order;
            tail = order;
        } else {
            tail->setNext(order);
            order->setPrev(tail);
            tail = order;
        }
        totalVolume += order->getQuantity();
    }

    void CancelOrder(OrderNode* order = nullptr) {
        totalVolume -= order->getQuantity();

        if (order->getPrev() != nullptr) {
            order->getPrev()->setNext(order->getNext());
        } else {
            head = order->getNext();
        }

        if (order->getNext() != nullptr) {
            order->getNext()->setPrev(order->getPrev());
        } else {
            tail = order->getPrev();
        }

    }
};

class OrderBook{
    unordered_map<uint64_t, OrderNode*>Hash;
    map<uint32_t, PriceLevel*, greater<uint32_t>>BidBook;
    map<uint32_t, PriceLevel*, less<uint32_t>>AskBook;
public:
    OrderBook() {};

    void ProcessOrder(OrderNode* newOrder) {
        if (newOrder->getOrderType() == BUY) {
            auto it = AskBook.begin();

            while (it != AskBook.end() && newOrder->getQuantity() > 0) {
                PriceLevel* bestAskLevel = it->second;

                if (bestAskLevel->getLimitPrice() > newOrder->getPrice()) {
                    break; 
                }

                OrderNode* restingOrder = bestAskLevel->getHead();
                while (restingOrder != nullptr && newOrder->getQuantity() > 0) {
                    
                    uint32_t tradeQty = min(newOrder->getQuantity(), restingOrder->getQuantity());

                    newOrder->reduceQuantity(tradeQty); 
                    restingOrder->reduceQuantity(tradeQty);

                    if (restingOrder->getQuantity() == 0) {
                        OrderNode* orderToRemove = restingOrder;
                        restingOrder = restingOrder->getNext(); 
                        
                        bestAskLevel->CancelOrder(orderToRemove);
                        Hash.erase(orderToRemove->getOrderId());
                        delete orderToRemove;
                    } else {
                        break; 
                    }
                }

                if (bestAskLevel->getTotalVolume() == 0) {
                    auto toErase = it;
                    it++;
                    AskBook.erase(toErase);
                    delete bestAskLevel;
                } else {
                    break; 
                }
            }
        } 
        else {
            auto it = BidBook.begin();

            while (it != BidBook.end() && newOrder->getQuantity() > 0) {
                PriceLevel* bestBidLevel = it->second;

                if (bestBidLevel->getLimitPrice() < newOrder->getPrice()) {
                    break;
                }

                OrderNode* restingOrder = bestBidLevel->getHead();
                while (restingOrder != nullptr && newOrder->getQuantity() > 0) {
                    
                    uint32_t tradeQty = min(newOrder->getQuantity(), restingOrder->getQuantity());

                    newOrder->reduceQuantity(tradeQty);
                    restingOrder->reduceQuantity(tradeQty);

                    if (restingOrder->getQuantity() == 0) {
                        OrderNode* orderToRemove = restingOrder;
                        restingOrder = restingOrder->getNext();
                        
                        bestBidLevel->CancelOrder(orderToRemove);
                        Hash.erase(orderToRemove->getOrderId());
                        delete orderToRemove;
                    } else {
                        break;
                    }
                }

                if (bestBidLevel->getTotalVolume() == 0) {
                    auto toErase = it;
                    it++;
                    BidBook.erase(toErase);
                    delete bestBidLevel;
                } else {
                    break;
                }
            }
        }

        if (newOrder->getQuantity() > 0) {
            AddOrder(newOrder); 
        } else {
            delete newOrder;
        }
    }

    void AddOrder(OrderNode* order){
        Hash[order->getOrderId()] = order;

        if(order->getOrderType() == OrderType::BUY){
            if(BidBook.find(order->getPrice()) == BidBook.end()){
                BidBook[order->getPrice()] = new PriceLevel(order->getPrice());
            }
            BidBook[order->getPrice()]->AddOrder(order);
        }else{
            if(AskBook.find(order->getPrice()) == AskBook.end()){
                AskBook[order->getPrice()] = new PriceLevel(order->getPrice());
            }
            AskBook[order->getPrice()]->AddOrder(order);
        }

    }

    void CancelOrder(uint64_t orderId){
        auto it = Hash.find(orderId);
        if(it == Hash.end()){
            cerr << "Order " << orderId << " not found for cancellation." << endl;
            return;
        }

        OrderNode* order = it->second;  

        if(order->getOrderType() == OrderType::BUY){
            PriceLevel* level = BidBook[order->getPrice()];
            level->CancelOrder(order);

            if(level->getTotalVolume() == 0){
                BidBook.erase(order->getPrice());
                delete level;
            }

        }else{
            PriceLevel* level = AskBook[order->getPrice()];
            level->CancelOrder(order);

            if(level->getTotalVolume() == 0){
                AskBook.erase(order->getPrice());
                delete level;
            }
        }

        Hash.erase(orderId);
        delete order;
    }
};
