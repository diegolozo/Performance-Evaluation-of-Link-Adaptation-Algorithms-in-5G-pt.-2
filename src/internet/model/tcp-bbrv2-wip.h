/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Vivek Jain <jain.vivek.anand@gmail.com>
 *          Viyom Mittal <viyommittal@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#ifndef TCPBBR_H
#define TCPBBR_H

#include "tcp-congestion-ops.h"
#include "windowed-filter.h"

#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"

class TcpBbrCheckGainValuesTest;

namespace ns3
{

/**
 * \ingroup congestionOps
 *
 * \brief BBR congestion control algorithm
 *
 * This class implement the BBR (Bottleneck Bandwidth and Round-trip propagation time)
 * congestion control type.
 */
class TcpBbr : public TcpCongestionOps
{
  public:
    /**
     * \brief The number of phases in the BBR ProbeBW gain cycle.
     */
    static const uint8_t GAIN_CYCLE_LENGTH = 8;

    /**
     * \brief BBR uses an eight-phase cycle with the given pacing_gain value
     * in the BBR ProbeBW gain cycle.
     */
    const static double PACING_GAIN_CYCLE[];
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor
     */
    TcpBbr();

    /**
     * Copy constructor.
     * \param sock The socket to copy from.
     */
    TcpBbr(const TcpBbr& sock);

    /**
     * \brief BBR has the following 4 modes for deciding how fast to send:
     */
    enum BbrMode_t
    {
        BBR_STARTUP,   /**< Ramp up sending rate rapidly to fill pipe */
        BBR_DRAIN,     /**< Drain any queue created during startup */
        BBR_PROBE_BW,  /**< Discover, share bw: pace around estimated bw */
        BBR_PROBE_RTT, /**< Cut inflight to min to probe min_rtt */
    };

    // ProbeBW Pacing Gain Phase Indexes
    typedef enum
    {
        BBR_BW_PROBE_UP = 0,
        BBR_BW_PROBE_DOWN = 1,
        BBR_BW_PROBE_CRUISE = 2,
        BBR_BW_PROBE_REFILL = 3,
        BBR_BW_PROBE_NS = 4
    } BbrBwPhase;

    // Relation between the ACK stream and the bandwidth probing
    typedef enum
    {
        BBR_ACKS_INIT,            // Not probing
        BBR_ACKS_REFILLING,       // Sending at estimated bw to fill pipe
        BBR_ACK_PROBE_STARTING,   // Inflight rising to probe bw
        BBR_ACK_PROBE_FEEDBACK,   // Getting feedback from bw probing
        BBR_ACK_PROBE_STOPPING    // Stopped probing
    } BbrAckPhase;

    typedef WindowedFilter<DataRate,
                           MaxFilter<DataRate>,
                           uint32_t,
                           uint32_t>
        MaxBandwidthFilter_t; //!< Definition of max bandwidth filter.

    // Different Variations of BBR
    // AGDT
    typedef enum
    {
        BBR,
        BBR_V2
    } BbrVar;

    /**
     * \brief Literal names of BBR mode for use in log messages
     */
    static const char* const BbrModeName[BBR_PROBE_RTT + 1];

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.
     *
     * \param stream first stream index to use
     */
    virtual void SetStream(uint32_t stream);

    std::string GetName() const override;
    bool HasCongControl() const override;
    void CongControl(Ptr<TcpSocketState> tcb,
                     const TcpRateOps::TcpRateConnection& rc,
                     const TcpRateOps::TcpRateSample& rs) override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;
    void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event) override;
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
    Ptr<TcpCongestionOps> Fork() override;

  protected:
    /**
     * \brief TcpBbrCheckGainValuesTest friend class (for tests).
     * \relates TcpBbrCheckGainValuesTest
     */
    friend class TcpBbrCheckGainValuesTest;

    /**
     * \brief Advances pacing gain using cycle gain algorithm, while in BBR_PROBE_BW state
     */
    void AdvanceCyclePhase();

    /**
     * \brief Checks whether to advance pacing gain in BBR_PROBE_BW state,
     *  and if allowed calls AdvanceCyclePhase ()
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void CheckCyclePhase(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Checks whether its time to enter BBR_DRAIN or BBR_PROBE_BW state
     * \param tcb the socket state.
     */
    void CheckDrain(Ptr<TcpSocketState> tcb);

    /**
     * \brief Identifies whether pipe or BDP is already full
     * \param rs rate sample.
     */
    void CheckFullPipe(const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief This method handles the steps related to the ProbeRTT state
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void CheckProbeRTT(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates variables specific to BBR_DRAIN state
     */
    void EnterDrain();

    /**
     * \brief Updates variables specific to BBR_PROBE_BW state
     */
    void EnterProbeBW(Ptr<TcpSocketState> tcb);

    /**
     * \brief Updates variables specific to BBR_PROBE_RTT state
     */
    void EnterProbeRTT();

    /**
     * \brief Updates variables specific to BBR_STARTUP state
     */
    void EnterStartup();

    /**
     * \brief Called on exiting from BBR_PROBE_RTT state, it eithers invoke EnterProbeBW () or
     * EnterStartup ()
     */
    void ExitProbeRTT(Ptr<TcpSocketState> tcb);

    /**
     * \brief Gets BBR state.
     * \return returns BBR state.
     */
    uint32_t GetBbrState();

    /**
     * \brief Gets current pacing gain.
     * \return returns current pacing gain.
     */
    double GetPacingGain();

    /**
     * \brief Gets current cwnd gain.
     * \return returns current cwnd gain.
     */
    double GetCwndGain();

    /**
     * \brief Handles the steps for BBR_PROBE_RTT state.
     * \param tcb the socket state.
     */
    void HandleProbeRTT(Ptr<TcpSocketState> tcb);

    /**
     * \brief Updates pacing rate if socket is restarting from idle state.
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void HandleRestartFromIdle(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Estimates the target value for congestion window
     * \param tcb  the socket state.
     * \param gain cwnd gain.
     * \return returns congestion window based on max bandwidth and min RTT.
     */
    uint32_t InFlight(Ptr<TcpSocketState> tcb, double gain);

    /**
     * \brief Initializes the full pipe estimator.
     */
    void InitFullPipe();

    /**
     * \brief Initializes the pacing rate.
     * \param tcb  the socket state.
     */
    void InitPacingRate(Ptr<TcpSocketState> tcb);

    /**
     * \brief Initializes the round counting related variables.
     */
    void InitRoundCounting();

    /**
     * \brief Checks whether to move to next value of pacing gain while in BBR_PROBE_BW.
     * \param tcb the socket state.
     * \param rs  rate sample.
     * \returns true if want to move to next value otherwise false.
     */
    bool IsNextCyclePhase(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Modulates congestion window in BBR_PROBE_RTT.
     * \param tcb the socket state.
     */
    void ModulateCwndForProbeRTT(Ptr<TcpSocketState> tcb);

    /**
     * \brief Modulates congestion window in CA_RECOVERY.
     * \param tcb the socket state.
     * \param rs rate sample.
     * \return true if congestion window is updated in CA_RECOVERY.
     */
    bool ModulateCwndForRecovery(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Helper to restore the last-known good congestion window
     * \param tcb the socket state.
     */
    void RestoreCwnd(Ptr<TcpSocketState> tcb);

    /**
     * \brief Helper to remember the last-known good congestion window or
     *        the latest congestion window unmodulated by loss recovery or ProbeRTT.
     * \param tcb the socket state.
     */
    void SaveCwnd(Ptr<const TcpSocketState> tcb);

    /**
     * \brief Updates congestion window based on the network model.
     * \param tcb the socket state.
     * \param rs  rate sample
     */
    void SetCwnd(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates pacing rate based on network model.
     * \param tcb the socket state.
     * \param gain pacing gain.
     */
    void SetPacingRate(Ptr<TcpSocketState> tcb, double gain);

    /**
     * \brief Updates send quantum based on the network model.
     * \param tcb the socket state.
     */
    void SetSendQuantum(Ptr<TcpSocketState> tcb);

    /**
     * \brief Updates maximum bottleneck.
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateBottleneckBandwidth(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates control parameters congestion windowm, pacing rate, send quantum.
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateControlParameters(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates BBR network model (Maximum bandwidth and minimum RTT).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateModelAndState(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates round counting related variables.
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateRound(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Updates minimum RTT.
     * \param tcb the socket state.
     */
    void UpdateRTprop(Ptr<TcpSocketState> tcb);

    /**
     * \brief Updates target congestion window.
     * \param tcb the socket state.
     */
    void UpdateTargetCwnd(Ptr<TcpSocketState> tcb);

    /**
     * \brief Sets BBR state.
     * \param state BBR state.
     */
    void SetBbrState(BbrMode_t state);

    /**
     * \brief Find Cwnd increment based on ack aggregation.
     * \return uint32_t aggregate cwnd.
     */
    uint32_t AckAggregationCwnd();

    /**
     * \brief Estimates max degree of aggregation.
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateAckAggregation(Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /* -- BBRv2 -- */
    
    /**
     * \brief Update the congestion signals (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateCongestionSignals (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Checks if probing for bandwidth (BBRv2).
     */
    bool IsProbingBandwidth ();

    /**
     * \brief Update bw and inflight lower bounds (BBRv2).
     * \param tcb the socket state.
     */
    void AdaptLowerBounds (Ptr<TcpSocketState> tcb);

    /**
     * \brief Update bw and inflight upper bounds (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    bool AdaptUpperBounds (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Check the loss rate in Startup (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void CheckExcessiveLossStartup (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Enter the probe down phase of ProbeBW (BBRv2).
     * \param tcb the socket state.
     */
    void EnterProbeDown (Ptr<TcpSocketState> tcb);

    /**
     * \brief Enter the probe refill phase of ProbeBW (BBRv2).
     * \param tcb the socket state.
     * \param bwProbeUpRounds the number of probe up rounds.
     */
    void EnterProbeRefill (Ptr<TcpSocketState> tcb, uint32_t bwProbeUpRounds);

    /**
     * \brief Enter the probe up phase of ProbeBW (BBRv2).
     * \param tcb the socket state.
     */
    void EnterProbeUp (Ptr<TcpSocketState> tcb);

    /**
     * \brief Enter the probe cruise phase of ProbeBW (BBRv2).
     */
    void EnterProbeCruise ();

    /**
     * \brief Update the ProbeBW phase (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateCyclePhase (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Each ProbeBw cycle flip bw filter window (BBRv2).
     */
    void AdvanceBwMaxFilter ();

    /**
     * \brief Set the target Inflight to the adapted BDP (BBRv2).
     */
    uint32_t TargetInflight ();

    /**
     * \brief Raise inflight_hi when no loss (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void ProbeInflightHighUpward (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs);

    /**
     * \brief Double the probing data every round trip of Probe Up in ProbeBW (BBRv2).
     * \param tcb the socket state.
     */
    void RaiseInflightHiSlope (Ptr<TcpSocketState> tcb);

    /**
     * \brief Check if it is time to probe for bw (BBRv2).
     * \param tcb the socket state.
     */
    bool IsTimeToProbe (Ptr<TcpSocketState> tcb);

    /** 
     * \brief Check if it is time to transition to Probe Cruise (BBRv2).
     * \param tcb the socket state.
     */
    bool IsTimeToCruise (Ptr<TcpSocketState> tcb);

    /**
     * \brief Ensure that it is safe to probe to keep coexistence with Reno (BBRv2).
     */
    bool IsTimeToProbeRenoCoexistence ();

    /**
     * \brief Returns an operating point that leaves headroom for other flows when buffer is overfilled (BBRv2).
     */
    uint32_t InflightWithHeadroom ();

    /**
     * \brief Bound cwnd to a sensible level based on probing state, phase and inflight model (BBRv2).
     * \param tcb the socket state.
     */
    void BoundCwndForInflightModel (Ptr<TcpSocketState> tcb);

    /**
     * \brief Update Ecn parameters (BBRv2).
     * \param tcb the socket state.
     * \param rs rate sample.
     */
    void UpdateEcn (Ptr<TcpSocketState> tcb, const TcpSocketBase& rs);

    /**
     * \brief Check if ECN mark rate is above threshold in Startup
     * \param tcb the socket state.
     * \param rs rate sample.
     * \param ratio the ratio of ece bytes delivered to total bytes delivered.
     */
    void CheckEcnTooHighStartup (Ptr<TcpSocketState> tcb, const TcpRateOps::TcpRateSample& rs, uint32_t ratio);




  private:
    BbrMode_t m_state{BbrMode_t::BBR_STARTUP}; //!< Current state of BBR state machine
    MaxBandwidthFilter_t m_maxBwFilter;        //!< Maximum bandwidth filter
    uint32_t m_bandwidthWindowLength{0}; //!< A constant specifying the length of the BBR.BtlBw max
                                         //!< filter window, default 10 packet-timed round trips.
    double m_pacingGain{0};              //!< The dynamic pacing gain factor
    double m_cWndGain{0};                //!< The dynamic congestion window gain factor
    double m_highGain{0};       //!< A constant specifying highest gain factor, default is 2.89
    bool m_isPipeFilled{false}; //!< A boolean that records whether BBR has filled the pipe
    uint32_t m_minPipeCwnd{
        0}; //!< The minimal congestion window value BBR tries to target, default 4 Segment size
    uint32_t m_roundCount{0}; //!< Count of packet-timed round trips
    bool m_roundStart{false}; //!< A boolean that BBR sets to true once per packet-timed round trip
    uint32_t m_nextRoundDelivered{0};           //!< Denotes the end of a packet-timed round trip
    Time m_probeRttDuration{MilliSeconds(200)}; //!< A constant specifying the minimum duration for
                                                //!< which ProbeRTT state, default 200 millisecs
    Time m_probeRtPropStamp{
        Seconds(0)}; //!< The wall clock time at which the current BBR.RTProp sample was obtained.
    Time m_probeRttDoneStamp{Seconds(0)}; //!< Time to exit from BBR_PROBE_RTT state
    bool m_probeRttRoundDone{false};      //!< True when it is time to exit BBR_PROBE_RTT
    bool m_packetConservation{false};     //!< Enable/Disable packet conservation mode
    uint32_t m_priorCwnd{0};              //!< The last-known good congestion window
    bool m_idleRestart{false};            //!< When restarting from idle, set it true
    uint32_t m_targetCWnd{0}; //!< Target value for congestion window, adapted to the estimated BDP
    DataRate m_fullBandwidth{0};      //!< Value of full bandwidth recorded
    uint32_t m_fullBandwidthCount{0}; //!< Count of full bandwidth recorded consistently
    Time m_minRtt{
        Time::Max()}; //!< Estimated two-way round-trip propagation delay of the path, estimated
                      //!< from the windowed minimum recent round-trip delay sample.
    uint32_t m_sendQuantum{
        0}; //!< The maximum size of a data aggregate scheduled and transmitted together
    Time m_cycleStamp{Seconds(0)};       //!< Last time gain cycle updated
    uint32_t m_cycleIndex{0};            //!< Current index of gain cycle
    bool m_minRttExpired{false};         //!< A boolean recording whether the BBR.RTprop has expired
    Time m_minRttFilterLen{Seconds(10)}; //!< A constant specifying the length of the RTProp min
                                         //!< filter window, default 10 secs.
    Time m_minRttStamp{
        Seconds(0)}; //!< The wall clock time at which the current BBR.RTProp sample was obtained
    bool m_isInitialized{false}; //!< Set to true after first time initializtion variables
    Ptr<UniformRandomVariable> m_uv{nullptr}; //!< Uniform Random Variable
    uint64_t m_delivered{0}; //!< The total amount of data in bytes delivered so far
    uint32_t m_appLimited{
        0}; //!< The index of the last transmitted packet marked as application-limited
    uint32_t m_extraAckedGain{1};         //!< Gain factor for adding extra ack to cwnd
    uint32_t m_extraAcked[2]{0, 0};       //!< Maximum excess data acked in epoch
    uint32_t m_extraAckedWinRtt{0};       //!< Age of extra acked in rtt
    uint32_t m_extraAckedWinRttLength{5}; //!< Window length of extra acked window
    uint32_t m_ackEpochAckedResetThresh{
        1 << 17}; //!< Max allowed val for m_ackEpochAcked, after which sampling epoch is reset
    uint32_t m_extraAckedIdx{0};     //!< Current index in extra acked array
    Time m_ackEpochTime{Seconds(0)}; //!< Starting of ACK sampling epoch time
    uint32_t m_ackEpochAcked{0};     //!< Bytes ACked in sampling epoch
    bool m_hasSeenRtt{false};        //!< Have we seen RTT sample yet?
    BbrVar      m_variant                     {BbrVar::BBR};       //!< Variant of BBR
    uint32_t    m_lossInRound                 {0};                 //!< The number of losses in the current round trip (BBRv2)
    uint32_t    m_ecnInRound                  {0};                 //!< The indicator for an ECN mark in the current round trip (BBRv2)
    uint32_t    m_lossInCycle                 {0};                 //!< The number of losses in the current ProbeBW cycle (BBRv2)
    uint32_t    m_ecnInCycle                  {0};                 //!< The indicator for an ECN mark in the current ProbeBW cycle (BBRv2)
    uint32_t    m_startupLossEvents           {0};                 //!< The number of loss events in the current round trip for Startup (BBRv2)
    uint32_t    m_lossRoundDelivered          {0};                 //!< The delivered at the end of a loss round (BBRv2)
    uint32_t    m_lossRoundStart              {0};                 //!< The indicator of having delivered at the end of a loss round (BBRv2)
    double      m_bbrBeta                     {3/10};              //!< Factor used to scale down inflight and rate for losses (BBRv2)
    uint32_t    m_fullLossCount               {8};                 //!< The number of loss marking events in Startup, exits Startup after >= N (BBRv2)
    double      m_lossThresh                  {2/100};             //!< Loss threshold for inflight data (BBRv2)
    uint32_t    m_bwProbeMaxRounds            {63};                //!< Max number of rounds to wait before probing for bandwidth (BBRv2)
    uint32_t    m_bwProbeRandRounds           {2};                 //!< Max amount of randomness to inject in round counting (BBRv2)
    uint32_t    m_roundsSinceProbe            {0};                 //!< The number of rounds since probe (BBRv2)
    uint32_t    m_bwProbeUpCount              {0};                 //!< The amount of packets delivered per increase in inflight_hi (BBRv2)
    uint32_t    m_bwProbeUpAcks               {0};                 //!< The amount of packets (S)Acked per increase in inflight_hi (BBRv2)
    uint32_t    m_bwProbeUpRounds             {0};                 //!< The number of rounds in Probe Up (BBRv2)
    BbrAckPhase m_ackPhase                    {BbrAckPhase::BBR_ACKS_INIT};  //!< The Ack phase in relation to bw probing (BBRv2)
    uint32_t    m_bwProbeSamples              {0};                 //!< The number of of rate samples reflected by the probed bw (BBRv2)
    bool        m_isRiskyProbe                {false};             //!< Indicates if the last Probe Up was stopped due to any risk (BBRv2)
    bool        m_prevProbeTooHigh            {false};             //!< Indicates if the last Probe Up went too high (BBRv2)
    double      m_inflightHeadroom            {15/100};            //!< The fraction of unutilised headroom to leave in path upon high loss (BBRv2)
    uint32_t    m_alphaLastDelivered          {0};                 //!< The previous delivered bytes when the ECN was updated (BBRv2)
    uint32_t    m_alphaLastDeliveredEce       {0};                 //!< The previous delivered bytes with ece marked when the ECN was updated (BBRv2)
    double      m_ecnFactor                   {1/3};               //!< The factor to reduce the inflight_lo once ECN is detected (BBRv2)
    double      m_ecnThresh                   {1/2};               //!< The threshold at which CE ratio is max before probing is too much (BBRv2)
    double      m_ecnGain                     {1/16};              //!< The factor for ECN mark ratio samples (BBRv2)
    uint32_t    m_ecnAlpha                    {1};                 //!< Ecn alpha value (BBRv2)
    uint32_t    m_fullEcnCount                {2};                 //!< The max number of of round trips that startup can go with Ecn rate over the threshold before exiting (BBRv2)
    uint32_t    m_startupEcnRounds            {0};                 //!< The number of round trips the Ecn rate has been above threshold in startup (BBRv2)
    bool        m_enableEcn                   {false};             //!< Use Ecn or not (BBRv2)
    uint64_t    m_deliveredEce                {0};                 //!< The total amount of data marked with ce delivered (BBRv2)
    uint32_t    m_deliveredEce_rs             {0};                 //!< The total amount of data marked with ce delivered (BBRv2) ! AGDT IMPLEMENTED; tcp-socket does no have this member anymore.
    bool        m_enableExp                   {false};             //!< Enable experimental changes for BBRv2
    uint32_t    m_bwProbeSampleOk             {0};                 //!< Indicates if the bw sample had low ECN/loss (BBRv2)
    bool        m_isEce                       {false};             //!< Set if an ECE was received in the latest ACK (BBRv2)
    uint32_t    m_inflightLo                  {std::numeric_limits<int>::max ()};                 //!< Lower (conservative) bound inflight data based on delivery/loss/ECN signals (BBRv2)
    uint32_t    m_inflightHi                  {std::numeric_limits<int>::max ()};                 //!< Upper (optimistic) bound inflight data based on delivery/loss/ECN signals (BBRv2)
    DataRate    m_bwLo                        {std::numeric_limits<int>::max ()};                 //!< Lower (conservative) bound bw rate based on delivery/loss/ECN signals (BBRv2)
    DataRate    m_bwHi                        {std::numeric_limits<int>::max ()};                 //!< Upper (optimistic) bound bw rate based on delivery/loss/ECN signals (BBRv2)
    DataRate    m_bwMax[2]                    {std::numeric_limits<int>::max (), std::numeric_limits<int>::max ()};       //!< Max recetn bw sample (BBRv2)
};

} // namespace ns3
#endif // TCPBBR_H
