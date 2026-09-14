#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "common/serialization_macros.h"
#include "serialization.h"
#include "serialization_impl.h"

namespace test
{
//=============================================================================
// Basic Building Blocks
//=============================================================================

class BusinessCenter
{
public:
    BusinessCenter() = default;
    explicit BusinessCenter(std::string center) : center_(std::move(center)) {}

    const std::string& center() const { return center_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(BusinessCenter, center_);
    std::string center_;
};

class BusinessCenters
{
public:
    BusinessCenters() = default;
    explicit BusinessCenters(std::vector<std::string> centers)
    {
        for (const auto& c : centers)
        {
            centers_.push_back(BusinessCenter(c));
        }
    }

    const auto& centers() const { return centers_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(BusinessCenters, centers_);
    std::vector<BusinessCenter> centers_;
};

class DateAdjustments
{
public:
    DateAdjustments() = default;
    DateAdjustments(std::string convention, std::vector<std::string> centers)
        : business_day_convention_(std::move(convention))
        , business_centers_(std::move(centers))
    {}

    const std::string& convention() const { return business_day_convention_; }
    const auto& centers() const { return business_centers_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(DateAdjustments, business_day_convention_, business_centers_);
    std::string      business_day_convention_;
    BusinessCenters  business_centers_;
};

class AdjustableDate
{
public:
    AdjustableDate() = default;
    AdjustableDate(std::string date, std::string convention, std::vector<std::string> centers)
        : unadjusted_date_(std::move(date))
        , date_adjustments_(std::move(convention), std::move(centers))
    {}

    const std::string& date() const { return unadjusted_date_; }
    const auto& adjustments() const { return date_adjustments_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(AdjustableDate, unadjusted_date_, date_adjustments_);
    std::string      unadjusted_date_;
    DateAdjustments  date_adjustments_;
};

class Frequency
{
public:
    Frequency() = default;
    Frequency(int multiplier, std::string period)
        : period_multiplier_(multiplier), period_(std::move(period))
    {}

    int multiplier() const { return period_multiplier_; }
    const std::string& period() const { return period_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(Frequency, period_multiplier_, period_);
    int         period_multiplier_{0};
    std::string period_;
};

//=============================================================================
// Calculation Period Dates
//=============================================================================

class CalculationPeriodDates
{
public:
    CalculationPeriodDates() = default;
    CalculationPeriodDates(
        std::string id,
        AdjustableDate effective_date,
        AdjustableDate termination_date,
        DateAdjustments adjustments,
        Frequency frequency,
        int roll_convention)
        : id_(std::move(id))
        , effective_date_(std::move(effective_date))
        , termination_date_(std::move(termination_date))
        , calculation_period_dates_adjustments_(std::move(adjustments))
        , calculation_period_frequency_(std::move(frequency))
        , roll_convention_(roll_convention)
    {}

    const std::string& id() const { return id_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        CalculationPeriodDates,
        id_,
        effective_date_,
        termination_date_,
        calculation_period_dates_adjustments_,
        calculation_period_frequency_,
        roll_convention_);

    std::string              id_;
    AdjustableDate           effective_date_;
    AdjustableDate           termination_date_;
    DateAdjustments          calculation_period_dates_adjustments_;
    Frequency                calculation_period_frequency_;
    int                      roll_convention_{0};
};

//=============================================================================
// Payment Dates
//=============================================================================

class PaymentDates
{
public:
    PaymentDates() = default;
    PaymentDates(
        std::string calc_period_ref,
        Frequency frequency,
        std::string pay_relative_to,
        DateAdjustments adjustments)
        : calculation_period_dates_reference_(std::move(calc_period_ref))
        , payment_frequency_(std::move(frequency))
        , pay_relative_to_(std::move(pay_relative_to))
        , payment_dates_adjustments_(std::move(adjustments))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        PaymentDates,
        calculation_period_dates_reference_,
        payment_frequency_,
        pay_relative_to_,
        payment_dates_adjustments_);

    std::string      calculation_period_dates_reference_;
    Frequency        payment_frequency_;
    std::string      pay_relative_to_;
    DateAdjustments  payment_dates_adjustments_;
};

//=============================================================================
// Reset Dates (for floating leg)
//=============================================================================

class FixingDates
{
public:
    FixingDates() = default;
    FixingDates(
        int period_multiplier,
        std::string period,
        std::string day_type,
        std::string convention,
        std::vector<std::string> centers,
        std::string date_relative_to)
        : period_multiplier_(period_multiplier)
        , period_(std::move(period))
        , day_type_(std::move(day_type))
        , business_day_convention_(std::move(convention))
        , business_centers_(std::move(centers))
        , date_relative_to_(std::move(date_relative_to))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        FixingDates,
        period_multiplier_,
        period_,
        day_type_,
        business_day_convention_,
        business_centers_,
        date_relative_to_);

    int              period_multiplier_{0};
    std::string      period_;
    std::string      day_type_;
    std::string      business_day_convention_;
    BusinessCenters  business_centers_;
    std::string      date_relative_to_;
};

class ResetDates
{
public:
    ResetDates() = default;
    ResetDates(
        std::string id,
        std::string calc_period_ref,
        std::string reset_relative_to,
        FixingDates fixing_dates,
        Frequency reset_frequency,
        DateAdjustments adjustments)
        : id_(std::move(id))
        , calculation_period_dates_reference_(std::move(calc_period_ref))
        , reset_relative_to_(std::move(reset_relative_to))
        , fixing_dates_(std::move(fixing_dates))
        , reset_frequency_(std::move(reset_frequency))
        , reset_dates_adjustments_(std::move(adjustments))
    {}

    const std::string& id() const { return id_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        ResetDates,
        id_,
        calculation_period_dates_reference_,
        reset_relative_to_,
        fixing_dates_,
        reset_frequency_,
        reset_dates_adjustments_);

    std::string      id_;
    std::string      calculation_period_dates_reference_;
    std::string      reset_relative_to_;
    FixingDates      fixing_dates_;
    Frequency        reset_frequency_;
    DateAdjustments  reset_dates_adjustments_;
};

//=============================================================================
// Calculation Period Amount
//=============================================================================

class NotionalStepSchedule
{
public:
    NotionalStepSchedule() = default;
    NotionalStepSchedule(double initial_value, std::string currency)
        : initial_value_(initial_value), currency_(std::move(currency))
    {}

    double value() const { return initial_value_; }
    const std::string& currency() const { return currency_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(NotionalStepSchedule, initial_value_, currency_);
    double      initial_value_{0.0};
    std::string currency_;
};

class NotionalSchedule
{
public:
    NotionalSchedule() = default;
    explicit NotionalSchedule(NotionalStepSchedule schedule)
        : notional_step_schedule_(std::move(schedule))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(NotionalSchedule, notional_step_schedule_);
    NotionalStepSchedule notional_step_schedule_;
};

class FixedRateSchedule
{
public:
    FixedRateSchedule() = default;
    explicit FixedRateSchedule(double rate) : initial_value_(rate) {}

    double rate() const { return initial_value_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(FixedRateSchedule, initial_value_);
    double initial_value_{0.0};
};

class FloatingRateCalculation
{
public:
    FloatingRateCalculation() = default;
    FloatingRateCalculation(
        std::string floating_rate_index,
        Frequency index_tenor,
        double spread)
        : floating_rate_index_(std::move(floating_rate_index))
        , index_tenor_(std::move(index_tenor))
        , spread_schedule_initial_value_(spread)
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        FloatingRateCalculation,
        floating_rate_index_,
        index_tenor_,
        spread_schedule_initial_value_);

    std::string floating_rate_index_;
    Frequency   index_tenor_;
    double      spread_schedule_initial_value_{0.0};
};

class FixedCalculation
{
public:
    FixedCalculation() = default;
    FixedCalculation(
        NotionalSchedule notional,
        FixedRateSchedule rate,
        std::string day_count)
        : notional_schedule_(std::move(notional))
        , fixed_rate_schedule_(std::move(rate))
        , day_count_fraction_(std::move(day_count))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        FixedCalculation,
        notional_schedule_,
        fixed_rate_schedule_,
        day_count_fraction_);

    NotionalSchedule   notional_schedule_;
    FixedRateSchedule  fixed_rate_schedule_;
    std::string        day_count_fraction_;
};

class FloatingCalculation
{
public:
    FloatingCalculation() = default;
    FloatingCalculation(
        NotionalSchedule notional,
        FloatingRateCalculation floating_rate,
        std::string day_count)
        : notional_schedule_(std::move(notional))
        , floating_rate_calculation_(std::move(floating_rate))
        , day_count_fraction_(std::move(day_count))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        FloatingCalculation,
        notional_schedule_,
        floating_rate_calculation_,
        day_count_fraction_);

    NotionalSchedule         notional_schedule_;
    FloatingRateCalculation  floating_rate_calculation_;
    std::string              day_count_fraction_;
};

class CalculationPeriodAmount
{
public:
    CalculationPeriodAmount() = default;
    explicit CalculationPeriodAmount(FixedCalculation calc)
        : calculation_(std::move(calc))
    {}
    explicit CalculationPeriodAmount(FloatingCalculation calc)
        : calculation_(std::move(calc))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(CalculationPeriodAmount, calculation_);
    std::variant<FixedCalculation, FloatingCalculation> calculation_;
};

//=============================================================================
// Swap Stream
//=============================================================================

class SwapStream
{
public:
    SwapStream() = default;

    // Constructor for fixed leg
    SwapStream(
        std::string id,
        std::string payer_ref,
        std::string receiver_ref,
        CalculationPeriodDates calc_period_dates,
        PaymentDates payment_dates,
        CalculationPeriodAmount calc_amount)
        : id_(std::move(id))
        , payer_party_reference_(std::move(payer_ref))
        , receiver_party_reference_(std::move(receiver_ref))
        , calculation_period_dates_(std::move(calc_period_dates))
        , payment_dates_(std::move(payment_dates))
        , calculation_period_amount_(std::move(calc_amount))
        , has_reset_dates_(false)
    {}

    // Constructor for floating leg (with reset dates)
    SwapStream(
        std::string id,
        std::string payer_ref,
        std::string receiver_ref,
        CalculationPeriodDates calc_period_dates,
        PaymentDates payment_dates,
        ResetDates reset_dates,
        CalculationPeriodAmount calc_amount)
        : id_(std::move(id))
        , payer_party_reference_(std::move(payer_ref))
        , receiver_party_reference_(std::move(receiver_ref))
        , calculation_period_dates_(std::move(calc_period_dates))
        , payment_dates_(std::move(payment_dates))
        , reset_dates_(std::move(reset_dates))
        , calculation_period_amount_(std::move(calc_amount))
        , has_reset_dates_(true)
    {}

    const std::string& id() const { return id_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        SwapStream,
        id_,
        payer_party_reference_,
        receiver_party_reference_,
        calculation_period_dates_,
        payment_dates_,
        reset_dates_,
        calculation_period_amount_,
        has_reset_dates_);

    std::string              id_;
    std::string              payer_party_reference_;
    std::string              receiver_party_reference_;
    CalculationPeriodDates   calculation_period_dates_;
    PaymentDates             payment_dates_;
    ResetDates               reset_dates_;
    CalculationPeriodAmount  calculation_period_amount_;
    bool                     has_reset_dates_{false};
};

//=============================================================================
// Trade Structure
//=============================================================================

class PartyTradeIdentifier
{
public:
    PartyTradeIdentifier() = default;
    PartyTradeIdentifier(std::string party_ref, std::string trade_id, std::string scheme)
        : party_reference_(std::move(party_ref))
        , trade_id_(std::move(trade_id))
        , trade_id_scheme_(std::move(scheme))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(PartyTradeIdentifier, party_reference_, trade_id_, trade_id_scheme_);
    std::string party_reference_;
    std::string trade_id_;
    std::string trade_id_scheme_;
};

class TradeHeader
{
public:
    TradeHeader() = default;
    TradeHeader(
        std::vector<PartyTradeIdentifier> identifiers,
        std::string trade_date)
        : party_trade_identifiers_(std::move(identifiers))
        , trade_date_(std::move(trade_date))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(TradeHeader, party_trade_identifiers_, trade_date_);
    std::vector<PartyTradeIdentifier> party_trade_identifiers_;
    std::string                       trade_date_;
};

class Swap
{
public:
    Swap() = default;
    Swap(std::vector<SwapStream> streams)
        : swap_streams_(std::move(streams))
    {}

    const auto& streams() const { return swap_streams_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(Swap, swap_streams_);
    std::vector<SwapStream> swap_streams_;
};

class Trade
{
public:
    Trade() = default;
    Trade(TradeHeader header, Swap swap)
        : trade_header_(std::move(header))
        , swap_(std::move(swap))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(Trade, trade_header_, swap_);
    TradeHeader trade_header_;
    Swap        swap_;
};

//=============================================================================
// Header and Party
//=============================================================================

class MessageId
{
public:
    MessageId() = default;
    MessageId(std::string id, std::string scheme)
        : message_id_(std::move(id)), scheme_(std::move(scheme))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(MessageId, message_id_, scheme_);
    std::string message_id_;
    std::string scheme_;
};

class Header
{
public:
    Header() = default;
    Header(
        MessageId message_id,
        std::string sent_by,
        std::string send_to,
        std::string creation_timestamp)
        : message_id_(std::move(message_id))
        , sent_by_(std::move(sent_by))
        , send_to_(std::move(send_to))
        , creation_timestamp_(std::move(creation_timestamp))
    {}

protected:
    void initialize() {};
    SERIALIZATION_MACRO(Header, message_id_, sent_by_, send_to_, creation_timestamp_);
    MessageId   message_id_;
    std::string sent_by_;
    std::string send_to_;
    std::string creation_timestamp_;
};

class Party
{
public:
    Party() = default;
    Party(std::string id, std::string party_id, std::string scheme, std::string name)
        : id_(std::move(id))
        , party_id_(std::move(party_id))
        , party_id_scheme_(std::move(scheme))
        , party_name_(std::move(name))
    {}

    const std::string& id() const { return id_; }
    const std::string& name() const { return party_name_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(Party, id_, party_id_, party_id_scheme_, party_name_);
    std::string id_;
    std::string party_id_;
    std::string party_id_scheme_;
    std::string party_name_;
};

//=============================================================================
// Top Level FpML
//=============================================================================

class test_fpml_swap
{
public:
    test_fpml_swap() = default;

    test_fpml_swap(
        Header header,
        bool is_correction,
        std::string correlation_id,
        int sequence_number,
        Trade trade,
        std::vector<Party> parties)
        : header_(std::move(header))
        , is_correction_(is_correction)
        , correlation_id_(std::move(correlation_id))
        , sequence_number_(sequence_number)
        , trade_(std::move(trade))
        , parties_(std::move(parties))
    {}

    const Header& header() const { return header_; }
    const Trade& trade() const { return trade_; }
    const auto& parties() const { return parties_; }

protected:
    void initialize() {};
    SERIALIZATION_MACRO(
        test_fpml_swap,
        header_,
        is_correction_,
        correlation_id_,
        sequence_number_,
        trade_,
        parties_);

    Header              header_;
    bool                is_correction_{false};
    std::string         correlation_id_;
    int                 sequence_number_{0};
    Trade               trade_;
    std::vector<Party>  parties_;
};

}  // namespace test

//=============================================================================
// Tests
//=============================================================================

class FpmlSerializationTest : public ::testing::Test
{
protected:
    pugi::xml_document doc;
    pugi::xml_node     buffer;

    void SetUp() override
    {
        doc.reset();
        buffer = doc.append_child("test");
    }
};

TEST_F(FpmlSerializationTest, SimplePartyTest)
{
    // Create a simple party
    test::Party party1("party1", "BANKXYZ123", "http://example.com", "Bank XYZ");

    // Serialize
    pugi::xml_document save_doc;
    auto root_node = save_doc.append_child("Party");
    serialization::save(root_node, party1);

    std::string xml_str;
    {
        std::ostringstream oss;
        save_doc.save(oss, "  ");
        xml_str = oss.str();
    }
    std::cout << "Serialized XML:\n" << xml_str << std::endl;

    // Deserialize
    test::Party party_loaded;
    serialization::load(root_node, party_loaded);

    // Verify
    EXPECT_EQ(party_loaded.id(), "party1");
    EXPECT_EQ(party_loaded.name(), "Bank XYZ");
}

TEST_F(FpmlSerializationTest, NestedHeaderTest)
{
    // First test MessageId alone
    test::MessageId msg_id("MSG12345", "http://www.example.com/messageId");

    pugi::xml_document msg_doc;
    auto msg_root = msg_doc.append_child("MessageId");
    serialization::save(msg_root, msg_id);

    std::string msg_xml;
    {
        std::ostringstream oss;
        msg_doc.save(oss, "  ");
        msg_xml = oss.str();
    }
    std::cout << "MessageId alone XML:\n" << msg_xml << std::endl;

    // Now test Header with MessageId
    test::Header header(msg_id, "BANKXYZ", "CLIENTABC", "2024-12-15T10:30:00Z");

    // Serialize
    pugi::xml_document save_doc;
    auto root_node = save_doc.append_child("Header");
    serialization::save(root_node, header);

    std::string xml_str;
    {
        std::ostringstream oss;
        save_doc.save(oss, "  ");
        xml_str = oss.str();
    }
    std::cout << "Header XML:\n" << xml_str << std::endl;

    // Deserialize
    test::Header header_loaded;
    serialization::load(root_node, header_loaded);
}

TEST_F(FpmlSerializationTest, FpmlSwapCreationAndSerialization)
{
    // Create the FpML swap structure programmatically

    // Header
    test::MessageId msg_id("MSG12345", "http://www.example.com/messageId");
    test::Header header(
        std::move(msg_id),
        "BANKXYZ",
        "CLIENTABC",
        "2024-12-15T10:30:00Z");

    // Trade Header
    std::vector<test::PartyTradeIdentifier> trade_identifiers;
    trade_identifiers.emplace_back("party1", "TRADE123456", "http://www.example.com/tradeId");
    trade_identifiers.emplace_back("party2", "TRADE654321", "http://www.example.com/tradeId");
    test::TradeHeader trade_header(std::move(trade_identifiers), "2024-12-13");

    // Fixed Leg
    test::CalculationPeriodDates fixed_calc_dates(
        "fixedCalcPeriodDates",
        test::AdjustableDate(
            "2024-12-17",
            "MODFOLLOWING",
            {"USNY", "GBLO"}),
        test::AdjustableDate(
            "2029-12-17",
            "MODFOLLOWING",
            {"USNY", "GBLO"}),
        test::DateAdjustments("MODFOLLOWING", {"USNY", "GBLO"}),
        test::Frequency(6, "M"),
        17);

    test::PaymentDates fixed_payment_dates(
        "fixedCalcPeriodDates",
        test::Frequency(6, "M"),
        "CalculationPeriodEndDate",
        test::DateAdjustments("MODFOLLOWING", {"USNY", "GBLO"}));

    test::NotionalStepSchedule notional(10000000.00, "USD");
    test::FixedCalculation fixed_calc(
        test::NotionalSchedule(notional),
        test::FixedRateSchedule(0.0425),
        "30/360");

    test::SwapStream fixed_leg(
        "fixedLeg",
        "party1",
        "party2",
        std::move(fixed_calc_dates),
        std::move(fixed_payment_dates),
        test::CalculationPeriodAmount(std::move(fixed_calc)));

    // Floating Leg
    test::CalculationPeriodDates floating_calc_dates(
        "floatingCalcPeriodDates",
        test::AdjustableDate(
            "2024-12-17",
            "MODFOLLOWING",
            {"USNY", "GBLO"}),
        test::AdjustableDate(
            "2029-12-17",
            "MODFOLLOWING",
            {"USNY", "GBLO"}),
        test::DateAdjustments("MODFOLLOWING", {"USNY", "GBLO"}),
        test::Frequency(3, "M"),
        17);

    test::PaymentDates floating_payment_dates(
        "floatingCalcPeriodDates",
        test::Frequency(3, "M"),
        "CalculationPeriodEndDate",
        test::DateAdjustments("MODFOLLOWING", {"USNY", "GBLO"}));

    test::FixingDates fixing_dates(
        -2,
        "D",
        "Business",
        "NONE",
        {"GBLO"},
        "resetDates");

    test::ResetDates reset_dates(
        "resetDates",
        "floatingCalcPeriodDates",
        "CalculationPeriodStartDate",
        std::move(fixing_dates),
        test::Frequency(3, "M"),
        test::DateAdjustments("MODFOLLOWING", {"USNY", "GBLO"}));

    test::NotionalStepSchedule floating_notional(10000000.00, "USD");
    test::FloatingCalculation floating_calc(
        test::NotionalSchedule(floating_notional),
        test::FloatingRateCalculation(
            "USD-LIBOR-BBA",
            test::Frequency(3, "M"),
            0.0000),
        "ACT/360");

    test::SwapStream floating_leg(
        "floatingLeg",
        "party2",
        "party1",
        std::move(floating_calc_dates),
        std::move(floating_payment_dates),
        std::move(reset_dates),
        test::CalculationPeriodAmount(std::move(floating_calc)));

    // Create Swap with both legs
    std::vector<test::SwapStream> streams;
    streams.push_back(std::move(fixed_leg));
    streams.push_back(std::move(floating_leg));
    test::Swap swap(std::move(streams));

    // Create Trade
    test::Trade trade(std::move(trade_header), std::move(swap));

    // Create Parties
    std::vector<test::Party> parties;
    parties.emplace_back(
        "party1",
        "BANKXYZ123456789012",
        "http://www.fpml.org/coding-scheme/external/iso17442",
        "Bank XYZ");
    parties.emplace_back(
        "party2",
        "CLIENTABC12345678901",
        "http://www.fpml.org/coding-scheme/external/iso17442",
        "Client ABC Corporation");

    // Create FpML document
    test::test_fpml_swap fpml(
        std::move(header),
        false,
        "CORR67890",
        1,
        std::move(trade),
        std::move(parties));

    // Serialize to XML
    pugi::xml_document save_doc;
    auto root_node = save_doc.append_child("FpML");
    serialization::save(root_node, fpml);

    // Write to file
    serialization::serialization_impl::access::write_xml(
        "test_fpml_swap_output.xml", save_doc);

    // Read back and verify
    pugi::xml_document load_doc;
    serialization::serialization_impl::access::read_xml(
        "test_fpml_swap_output.xml", load_doc);

    test::test_fpml_swap fpml_loaded;
    auto loaded_root = load_doc.child("FpML");
    serialization::load(loaded_root, fpml_loaded);

    // Verify key data
    EXPECT_EQ(fpml_loaded.parties().size(), 2);
    EXPECT_EQ(fpml_loaded.parties()[0].name(), "Bank XYZ");
    EXPECT_EQ(fpml_loaded.parties()[1].name(), "Client ABC Corporation");
}
